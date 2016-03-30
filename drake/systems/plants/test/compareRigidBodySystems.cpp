
#include "drake/systems/plants/RigidBodySystem.h"
#include "drake/systems/plants/RigidBodyFrame.h"
#include "drake/util/testUtil.h"

using namespace std;
using namespace Eigen;
using namespace Drake;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " [options] full_path_to_robot1 full_path_to_robot2 x y z\n"
              << " The x y z parameters are optional and specify the position"
              << " of robot2 in the world, which is useful for URDF"
              << " models)"
              << std::endl;
    return 1;
  }

  std::shared_ptr<RigidBodyFrame> weld_to_frame;

  if (argc > 3) {
    weld_to_frame = allocate_shared<RigidBodyFrame>(
      aligned_allocator<RigidBodyFrame>(),
      "world",
      nullptr,  // not used since the robot is attached to the world
      Eigen::Vector3d(std::stod(argv[3]), std::stod(argv[4]), std::stod(argv[5])),  // xyz of the car's root link
      Eigen::Vector3d(0, 0, 0));       // rpy of the car's root link
  } else {
    weld_to_frame = allocate_shared<RigidBodyFrame>(
      aligned_allocator<RigidBodyFrame>(),
      "world",
      nullptr,
      Isometry3d::Identity());
  }

  std::cout << "Loading " << argv[1] << "..." << std::endl;
  auto r1 = make_shared<RigidBodySystem>();
  r1->addRobotFromFile(argv[1], DrakeJoint::QUATERNION);
  std::cout << "Loading " << argv[2] << "..." << std::endl;
  auto r2 = make_shared<RigidBodySystem>();
  r2->addRobotFromFile(argv[2], DrakeJoint::QUATERNION, weld_to_frame);

  // for debugging:
  r1->getRigidBodyTree()->drawKinematicTree("/tmp/r1.dot");
  r2->getRigidBodyTree()->drawKinematicTree("/tmp/r2.dot");
  // I ran this at the console to see the output:
  // dot -Tpng -O /tmp/r1.dot; dot -Tpng -O /tmp/r2.dot; open /tmp/r1.dot.png
  // /tmp/r2.dot.png

  try {
    valuecheck(r1->getNumStates(), r2->getNumStates());
  } catch(const std::exception& e) {
    std::cout << "ERROR: Number of states do not match!" << std::endl
              << "  - system 1: " << r1->getNumStates() << std::endl
              << "  - system 2: " << r2->getNumStates() << std::endl;
    return -1;
  }

  try {
    valuecheck(r1->getNumInputs(), r2->getNumInputs());
  } catch(const std::exception& e) {
    std::cout << "ERROR: Number of inputs do not match!" << std::endl
              << "  - system 1: " << r1->getNumInputs() << std::endl
              << "  - system 2: " << r2->getNumInputs() << std::endl;
    return -1;
  }

  try {
    valuecheck(r1->getNumOutputs(), r2->getNumOutputs());
  } catch(const std::exception& e) {
    std::cout << "ERROR: Number of outputs do not match!" << std::endl
              << "  - system 1: " << r1->getNumOutputs() << std::endl
              << "  - system 2: " << r2->getNumOutputs() << std::endl;
    return -1;
  }

  // Print the semantics of the states
  // std::cout << "State vector semantics (tree 1):\n" << r1->getStateVectorSemantics() << std::endl;
  // std::cout << "State vector semantics (tree 2):\n" << r2->getStateVectorSemantics() << std::endl;

  for (int i = 0; i < 1000; i++) {
    double t = 0.0;
    VectorXd x = getInitialState(*r1);
    VectorXd u = VectorXd::Random(r1->getNumInputs());

    auto xdot1 = r1->dynamics(t, x, u);
    auto xdot2 = r2->dynamics(t, x, u);
    try {
      valuecheckMatrix(xdot1, xdot2, 1e-8);
    } catch(const std::runtime_error& re) {
      std::cout << "Model mismatch!" << std::endl
                << "  - initial state:" << std::endl << x << std::endl
                << "  - inputs (joint torques?):" << std::endl << u << std::endl
                << "  - xdot1:" << std::endl << xdot1.transpose() << std::endl
                << "  - xdot2:" << std::endl << xdot2.transpose() << std::endl
                << "  - error message:" << std::endl << re.what() << std::endl;
      return -1;
    }
  }
}
