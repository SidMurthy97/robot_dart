#include <iostream>

#include <robot_dart/control/pd_control.hpp>
#include <robot_dart/robot_dart_simu.hpp>

#include <robot_dart/gui/magnum/graphics.hpp>
#include <robot_dart/gui/magnum/sensor/camera.hpp>

class MyApp : public robot_dart::gui::magnum::GlfwApplication {
public:
    explicit MyApp(int argc, char** argv, robot_dart::RobotDARTSimu* simu, const robot_dart::gui::magnum::GraphicsConfiguration& configuration = robot_dart::gui::magnum::GraphicsConfiguration())
        : GlfwApplication(argc, argv, simu, configuration)
    {
        // we synchronize by default if we have the graphics activated
        simu->scheduler().set_sync(true);
        // enable summary text when graphics activated
        simu->enable_text_panel(true);
        simu->enable_status_bar(true);
    }

protected:
    void keyPressEvent(KeyEvent& event) override
    {
        // One can customize their key/mouse events by inheriting GlfwApplication
        // if (event.key() == KeyEvent::Key::Right) {
        // }

        robot_dart::gui::magnum::GlfwApplication::keyPressEvent(event);
    }
};

int main()
{
    std::srand(std::time(NULL));

    std::vector<std::pair<std::string, std::string>> packages = {{"iiwa_description", "iiwa/iiwa_description"}};
    auto robot = std::make_shared<robot_dart::Robot>("iiwa/iiwa.urdf", packages);

    robot->fix_to_world();
    robot->set_position_enforced(true);

    Eigen::VectorXd ctrl = robot_dart::make_vector({0., M_PI / 3., 0., -M_PI / 4., 0., 0., 0.});

    auto controller = std::make_shared<robot_dart::control::PDControl>(ctrl);
    robot->add_controller(controller);
    controller->set_pd(500., 50.);

    robot_dart::RobotDARTSimu simu(0.001);

    // Magnum graphics
    robot_dart::gui::magnum::GraphicsConfiguration configuration = robot_dart::gui::magnum::Graphics::default_configuration();
    configuration.width = 1024;
    configuration.height = 768;
    auto graphics = std::make_shared<robot_dart::gui::magnum::BaseGraphics<MyApp>>(configuration);
    simu.set_graphics(graphics);
    graphics->look_at({0., 3.5, 2.}, {0., 0., 0.25});

    // record images from main camera/graphics
    graphics->camera().record(true);
    // we can also record a video directly to a file --- requires the executable of ffmpeg
    graphics->record_video("video-main.mp4", simu.graphics_freq());

    // Add camera
    auto camera = std::make_shared<robot_dart::sensor::Camera>(graphics->magnum_app(), 256, 256);
    camera->camera().set_far_plane(5.f);
    camera->camera().record(true, true); // cameras are recording color images by default, enable depth images as well for this example
    // cameras can also record video
    camera->record_video("video-camera.mp4");
    // camera->look_at({-0.5, -3., 0.75}, {0.5, 0., 0.2});
    Eigen::Isometry3d tf;
    // tf.setIdentity();
    tf = Eigen::AngleAxisd(3.14, Eigen::Vector3d{1., 0., 0.});
    tf *= Eigen::AngleAxisd(1.57, Eigen::Vector3d{0., 0., 1.});
    tf.translation() = Eigen::Vector3d(0., 0., 0.1);
    camera->attach_to_body(robot->body_node("iiwa_link_ee"), tf); // cameras are looking towards -z by default

    // simu.add_floor(5.);
    simu.add_checkerboard_floor(10.);
    simu.add_robot(robot);
    Eigen::Vector6d pose;
    pose << 0., 0., 0., 1.5, 0., 0.1;
    simu.add_robot(robot_dart::Robot::create_box({0.1, 0.1, 0.1}, pose, "free", 1., dart::Color::Red(1.), "box"));
    simu.add_sensor(camera);

    simu.run(10.);

    // a nested std::vector (w*h*3) of the last image taken can be retrieved
    auto gimage = graphics->image();
    auto cimage = camera->image();

    // we can also save them to png
    robot_dart::gui::save_png_image("camera-small.png", cimage);
    robot_dart::gui::save_png_image("camera-main.png", gimage);

    // convert an rgb image to grayscale (useful in some cases)
    auto gray_image = robot_dart::gui::convert_rgb_to_grayscale(gimage);
    robot_dart::gui::save_png_image("camera-gray.png", gray_image);

    // get the depth image from a camera
    // with a version for visualization or bigger differences in the output
    robot_dart::gui::save_png_image("camera-depth.png", camera->depth_image());
    // and the raw values that can be used along with the camera parameters to transform the image to point-cloud
    robot_dart::gui::save_png_image("camera-depth-raw.png", camera->raw_depth_image());

    robot.reset();
    return 0;
}
