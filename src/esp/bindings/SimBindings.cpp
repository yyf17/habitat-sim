// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "esp/bindings/bindings.h"

#include <Magnum/ImageView.h>
#include <Magnum/Magnum.h>
#include <Magnum/SceneGraph/SceneGraph.h>

#include <Magnum/PythonBindings.h>
#include <Magnum/SceneGraph/PythonBindings.h>

#include "esp/gfx/RenderCamera.h"
#include "esp/gfx/Renderer.h"
#include "esp/scene/SemanticScene.h"
#include "esp/sim/Simulator.h"

namespace py = pybind11;
using py::literals::operator""_a;

namespace esp {
namespace sim {

void initSimBindings(py::module& m) {
  // ==== SimulatorConfiguration ====
  py::class_<SimulatorConfiguration, SimulatorConfiguration::ptr>(
      m, "SimulatorConfiguration")
      .def(py::init(&SimulatorConfiguration::create<>))
      .def_readwrite("scene", &SimulatorConfiguration::scene)
      .def_readwrite("random_seed", &SimulatorConfiguration::randomSeed)
      .def_readwrite("default_agent_id",
                     &SimulatorConfiguration::defaultAgentId)
      .def_readwrite("default_camera_uuid",
                     &SimulatorConfiguration::defaultCameraUuid)
      .def_readwrite("gpu_device_id", &SimulatorConfiguration::gpuDeviceId)
      .def_readwrite("compress_textures",
                     &SimulatorConfiguration::compressTextures)
      .def_readwrite("allow_sliding", &SimulatorConfiguration::allowSliding)
      .def_readwrite("create_renderer", &SimulatorConfiguration::createRenderer)
      .def_readwrite("frustum_culling", &SimulatorConfiguration::frustumCulling)
      .def_readwrite("enable_physics", &SimulatorConfiguration::enablePhysics)
      .def_readwrite("physics_config_file",
                     &SimulatorConfiguration::physicsConfigFile)
      .def_readwrite("scene_light_setup",
                     &SimulatorConfiguration::sceneLightSetup)
      .def_readwrite("load_semantic_mesh",
                     &SimulatorConfiguration::loadSemanticMesh)
      .def(py::self == py::self)
      .def(py::self != py::self);

  // ==== Simulator ====
  py::class_<Simulator, Simulator::ptr>(m, "Simulator")
      .def(py::init<const SimulatorConfiguration&>())
      .def("get_active_scene_graph", &Simulator::getActiveSceneGraph,
           R"(PYTHON DOES NOT GET OWNERSHIP)",
           py::return_value_policy::reference)
      .def("get_active_semantic_scene_graph",
           &Simulator::getActiveSemanticSceneGraph,
           R"(PYTHON DOES NOT GET OWNERSHIP)",
           py::return_value_policy::reference)
      .def_property_readonly("semantic_scene", &Simulator::getSemanticScene, R"(
        The semantic scene graph

        .. note-warning::

            Not available for all datasets
            )")
      .def_property_readonly("renderer", &Simulator::getRenderer)
      .def("seed", &Simulator::seed, "new_seed"_a)
      .def("reconfigure", &Simulator::reconfigure, "configuration"_a)
      .def("reset", &Simulator::reset)
      .def("close", &Simulator::close)
      .def_property("pathfinder", &Simulator::getPathFinder,
                    &Simulator::setPathFinder)
      .def_property(
          "navmesh_visualization", &Simulator::isNavMeshVisualizationActive,
          &Simulator::setNavMeshVisualization,
          R"(Enable or disable wireframe visualization of current pathfinder's NavMesh.)")
      .def_property_readonly("gpu_device", &Simulator::gpuDevice)
      .def_property_readonly("random", &Simulator::random)
      .def_property("frustum_culling", &Simulator::isFrustumCullingEnabled,
                    &Simulator::setFrustumCullingEnabled,
                    R"(Enable or disable the frustum culling)")
      /* --- Physics functions --- */
      /* --- Template Manager accessors --- */
      .def(
          "get_asset_template_manager", &Simulator::getAssetAttributesManager,
          pybind11::return_value_policy::reference,
          R"(Get the Simulator's AssetAttributesManager instance for configuring primitive asset templates.)")
      .def(
          "get_object_template_manager", &Simulator::getObjectAttributesManager,
          pybind11::return_value_policy::reference,
          R"(Get the Simulator's ObjectAttributesManager instance for configuring object templates.)")
      .def(
          "get_physics_template_manager",
          &Simulator::getPhysicsAttributesManager,
          pybind11::return_value_policy::reference,
          R"(Get the Simulator's PhysicsAttributesManager instance for configuring PhysicsManager templates.)")
      .def(
          "get_stage_template_manager", &Simulator::getStageAttributesManager,
          pybind11::return_value_policy::reference,
          R"(Get the Simulator's StageAttributesManager instance for configuring simulation stage templates.)")
      .def(
          "get_physics_simulation_library",
          &Simulator::getPhysicsSimulationLibrary,
          R"(Query the physics library implementation currently configured by this Simulator instance.)")
      /* --- Object instancing and access --- */
      .def(
          "add_object", &Simulator::addObject, "object_lib_id"_a,
          "attachment_node"_a = nullptr,
          "light_setup_key"_a = assets::ResourceManager::DEFAULT_LIGHTING_KEY,
          "scene_id"_a = 0,
          R"(Instance an object into the scene via a template referenced by library id. Optionally attach the object to an existing SceneNode and assign its initial LightSetup key.)")
      .def(
          "add_object_by_handle", &Simulator::addObjectByHandle,
          "object_lib_handle"_a, "attachment_node"_a = nullptr,
          "light_setup_key"_a = assets::ResourceManager::DEFAULT_LIGHTING_KEY,
          "scene_id"_a = 0,
          R"(Instance an object into the scene via a template referenced by its handle. Optionally attach the object to an existing SceneNode and assign its initial LightSetup key.)")
      .def("remove_object", &Simulator::removeObject, "object_id"_a,
           "delete_object_node"_a = true, "delete_visual_node"_a = true,
           "scene_id"_a = 0, R"(
        Remove an object instance from the scene. Optionally leave its root SceneNode and visual SceneNode on the SceneGraph.

        .. note-warning::

            Removing an object which was attached to the SceneNode
            of an Agent expected to continue producing observations
            will likely result in errors if delete_object_node is true.
            )")
      .def(
          "get_object_initialization_template",
          &Simulator::getObjectInitializationTemplate, "object_id"_a,
          "scene_id"_a = 0,
          R"(Get a copy of the ObjectAttributes template used to instance an object.)")
      .def("get_object_motion_type", &Simulator::getObjectMotionType,
           "object_id"_a, "scene_id"_a = 0,
           R"(Get the MotionType of an object.)")
      .def("set_object_motion_type", &Simulator::setObjectMotionType,
           "motion_type"_a, "object_id"_a, "scene_id"_a = 0,
           R"(Set the MotionType of an object.)")
      .def(
          "get_existing_object_ids", &Simulator::getExistingObjectIDs,
          "scene_id"_a = 0,
          R"(Get the list of ids for all objects currently instanced in the scene.)")

      /* --- Kinematics and dynamics --- */
      .def(
          "step_world", &Simulator::stepWorld, "dt"_a = 1.0 / 60.0,
          R"(Step the physics simulation by a desired timestep (dt). Note that resulting world time after step may not be exactly t+dt. Use get_world_time to query current simulation time.)")
      .def("get_world_time", &Simulator::getWorldTime,
           R"(Query the current simualtion world time.)")
      .def("get_gravity", &Simulator::getGravity, "scene_id"_a = 0,
           R"(Query the gravity vector for a scene.)")
      .def("set_gravity", &Simulator::setGravity, "gravity"_a, "scene_id"_a = 0,
           R"(Set the gravity vector for a scene.)")
      .def(
          "get_object_scene_node", &Simulator::getObjectSceneNode,
          "object_id"_a, "scene_id"_a = 0,
          R"(Get a reference to the root SceneNode of an object's SceneGraph subtree.)")
      .def(
          "get_object_visual_scene_nodes",
          &Simulator::getObjectVisualSceneNodes, "object_id"_a,
          "scene_id"_a = 0,
          R"(Get a list of references to the SceneNodes with an object's render assets attached. Use this to manipulate the visual state of an object. Changes to these nodes will not affect physics simulation.)")
      .def(
          "set_transformation", &Simulator::setTransformation, "transform"_a,
          "object_id"_a, "scene_id"_a = 0,
          R"(Set the transformation matrix of an object's root SceneNode and update its simulation state.)")
      .def("get_transformation", &Simulator::getTransformation, "object_id"_a,
           "scene_id"_a = 0,
           R"(Get the transformation matrix of an object's root SceneNode.)")
      .def(
          "set_rigid_state", &Simulator::setRigidState, "rigid_state"_a,
          "object_id"_a, "scene_id"_a = 0,
          R"(Set the transformation of an object from a RigidState and update its simulation state.)")
      .def(
          "get_rigid_state", &Simulator::getRigidState, "object_id"_a,
          "scene_id"_a = 0,
          R"(Get an object's transformation as a RigidState (i.e. vector, quaternion).)")
      .def("set_translation", &Simulator::setTranslation, "translation"_a,
           "object_id"_a, "scene_id"_a = 0,
           R"(Set an object's translation and update its simulation state.)")
      .def("get_translation", &Simulator::getTranslation, "object_id"_a,
           "scene_id"_a = 0, R"(Get an object's translation.)")
      .def("set_rotation", &Simulator::setRotation, "rotation"_a, "object_id"_a,
           "scene_id"_a = 0,
           R"(Set an object's orientation and update its simulation state.)")
      .def("get_rotation", &Simulator::getRotation, "object_id"_a,
           "scene_id"_a = 0, R"(Get an object's orientation.)")
      .def(
          "get_object_velocity_control", &Simulator::getObjectVelocityControl,
          "object_id"_a, "scene_id"_a = 0,
          R"(Get a reference to an object's VelocityControl struct. Use this to set constant control velocities for MotionType::KINEMATIC and MotionType::DYNAMIC objects.)")
      .def(
          "set_linear_velocity", &Simulator::setLinearVelocity, "linVel"_a,
          "object_id"_a, "scene_id"_a = 0,
          R"(Set the linear component of an object's velocity. Only applies to MotionType::DYNAMIC objects.)")
      .def(
          "get_linear_velocity", &Simulator::getLinearVelocity, "object_id"_a,
          "scene_id"_a = 0,
          R"(Get the linear component of an object's velocity. Only non-zero for MotionType::DYNAMIC objects.)")
      .def(
          "set_angular_velocity", &Simulator::setAngularVelocity, "angVel"_a,
          "object_id"_a, "scene_id"_a = 0,
          R"(Set the angular component of an object's velocity. Only applies to MotionType::DYNAMIC objects.)")
      .def(
          "get_angular_velocity", &Simulator::getAngularVelocity, "object_id"_a,
          "scene_id"_a = 0,
          R"(Get the angular component of an object's velocity. Only non-zero for MotionType::DYNAMIC objects.)")
      .def(
          "apply_force", &Simulator::applyForce, "force"_a,
          "relative_position"_a, "object_id"_a, "scene_id"_a = 0,
          R"(Apply an external force to an object at a specific point relative to the object's center of mass in global coordinates. Only applies to MotionType::DYNAMIC objects.)")
      .def(
          "apply_torque", &Simulator::applyTorque, "torque"_a, "object_id"_a,
          "scene_id"_a = 0,
          R"(Apply torque to an object. Only applies to MotionType::DYNAMIC objects.)")
      .def(
          "contact_test", &Simulator::contactTest, "object_id"_a,
          "scene_id"_a = 0,
          R"(Run collision detection and return a binary indicator of penetration between the specified object and any other collision object. Physics must be enabled.)")
      .def(
          "cast_ray", &Simulator::castRay, "ray"_a, "max_distance"_a = 100.0,
          "scene_id"_a = 0,
          R"(Cast a ray into the collidable scene and return hit results. Physics must be enabled. max_distance in units of ray length.)")
      .def("set_object_bb_draw", &Simulator::setObjectBBDraw, "draw_bb"_a,
           "object_id"_a, "scene_id"_a = 0,
           R"(Enable or disable bounding box visualization for an object.)")
      .def(
          "set_object_semantic_id", &Simulator::setObjectSemanticId,
          "semantic_id"_a, "object_id"_a, "scene_id"_a = 0,
          R"(Convenience function to set the semanticId for all visual SceneNodes belonging to an object.)")
      .def(
          "recompute_navmesh", &Simulator::recomputeNavMesh, "pathfinder"_a,
          "navmesh_settings"_a, "include_static_objects"_a = false,
          R"(Recompute the NavMesh for a given PathFinder instance using configured NavMeshSettings. Optionally include all MotionType::STATIC objects in the navigability constraints.)")
      .def("get_light_setup", &Simulator::getLightSetup,
           "key"_a = assets::ResourceManager::DEFAULT_LIGHTING_KEY,
           R"(Get a copy of the LightSetup registered with a specific key.)")
      .def(
          "set_light_setup", &Simulator::setLightSetup, "light_setup"_a,
          "key"_a = assets::ResourceManager::DEFAULT_LIGHTING_KEY,
          R"(Register a LightSetup with a specific key. If a LightSetup is already registered with this key, it will be overriden. All Drawables referencing the key will use the newly registered LightSetup.)")
      .def(
          "set_object_light_setup", &Simulator::setObjectLightSetup,
          "object_id"_a, "light_setup_key"_a, "scene_id"_a = 0,
          R"(Modify the LightSetup used to the render all components of an object by setting the LightSetup key referenced by all Drawables attached to the object's visual SceneNodes.)");
}

}  // namespace sim
}  // namespace esp
