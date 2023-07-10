#pragma once

#include <teb_optim_local_planner/macros.h>

#include <ros/ros.h>

// base local planner base class and utilities
#include <nav_core/base_local_planner.h>
#include <mbf_costmap_core/costmap_controller.h>
#include <base_local_planner/goal_functions.h>
#include <base_local_planner/odometry_helper_ros.h>
#include <base_local_planner/costmap_model.h>


// timed-elastic-band related classes
#include <teb_local_planner/planner_interface.h>
#include <teb_local_planner/optimal_planner.h>
#include <teb_local_planner/homotopy_class_planner.h>
#include <teb_local_planner/visualization.h>
#include <teb_local_planner/recovery_behaviors.h>

// message types
#include <nav_msgs/Path.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/PoseStamped.h>
#include <visualization_msgs/MarkerArray.h>
#include <visualization_msgs/Marker.h>
#include <costmap_converter/ObstacleMsg.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Float32MultiArray.h>
#include <nifti_robot_driver_msgs/Tracks.h>

// transforms
#include <tf2/utils.h>
#include <tf2_ros/buffer.h>
#include <tf/transform_listener.h>

// costmap
#include <costmap_2d/costmap_2d_ros.h>
#include <costmap_converter/costmap_converter_interface.h>


// dynamic reconfigure
#include <teb_local_planner/TebLocalPlannerReconfigureConfig.h>
#include <dynamic_reconfigure/server.h>

// boost classes
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <actionlib/server/simple_action_server.h>
#include <actionlib/client/simple_action_client.h>

#include <trajectory_control_msgs/TrajectoryControlAction.h>
#include <trajectory_control_msgs/PlanningFeedback.h>
#include <trajectory_control_msgs/PlanningTask.h>
#include <trajectory_control_msgs/MultiRobotPath.h> 
#include <trajectory_control_msgs/message_enums.h> // for SegmentStatus and TaskType

#include <trajectory_control_msgs/VelocityCommand.h>


namespace teb_local_planner
{

/**
  * @class TebOptimLocalPlannerServer
  * @brief Implements teb_local_planner interface for 3D navigation stack
  * @todo Escape behavior, more efficient obstacle handling
  */

//NOTE: the following class was derived from TebLocalPlannerROS included in the repo https://github.com/rst-tu-dortmund/teb_local_planner
class TebOptimLocalPlannerServer 
{

public:
  /**
    * @brief Default constructor of the teb plugin
    */
  explicit TebOptimLocalPlannerServer(const std::string& name);

  /**
    * @brief  Destructor of the plugin
    */
  ~TebOptimLocalPlannerServer();

  /**
    * @brief Initializes the teb plugin
    * @param name The name of the instance
    * @param tf Pointer to a tf buffer
    * @param costmap_ros Cost map representing occupied and free space
    */
  void initialize();


  bool getRobotPose(tf::StampedTransform& robot_pose);
  bool getTransform(const std::string& target_frame, const std::string& source_frame, const ros::Time& target_time, tf::StampedTransform& transform);
  bool getTransform(const std::string& target_frame, const ros::Time& target_time,
                    const std::string& source_frame, const ros::Time& source_time,
                    const std::string& fixed_frame, tf::StampedTransform& transform);


  /**
    * @brief Set the plan that the teb local planner is following
    * @param orig_global_plan The plan to pass to the local planner
    * @return True if the plan was updated successfully, false otherwise
    */
  bool setPlan(const std::vector<geometry_msgs::PoseStamped>& orig_global_plan);

  /**
    * @brief Given the current position, orientation, and velocity of the robot, compute velocity commands to send to the base
    * @param cmd_vel Will be filled with the velocity command to be passed to the robot base
    * @return True if a valid trajectory was found, false otherwise
    */
  bool computeVelocityCommands(geometry_msgs::Twist& cmd_vel);

  /**
    * @brief Given the current position, orientation, and velocity of the robot, compute velocity commands to send to the base.
    * @remark Extended version for MBF API
    * @param pose the current pose of the robot.
    * @param velocity the current velocity of the robot.
    * @param cmd_vel Will be filled with the velocity command to be passed to the robot base.
    * @param message Optional more detailed outcome as a string
    * @return Result code as described on ExePath action result:
    *         SUCCESS         = 0
    *         1..9 are reserved as plugin specific non-error results
    *         FAILURE         = 100   Unspecified failure, only used for old, non-mfb_core based plugins
    *         CANCELED        = 101
    *         NO_VALID_CMD    = 102
    *         PAT_EXCEEDED    = 103
    *         COLLISION       = 104
    *         OSCILLATION     = 105
    *         ROBOT_STUCK     = 106
    *         MISSED_GOAL     = 107
    *         MISSED_PATH     = 108
    *         BLOCKED_PATH    = 109
    *         INVALID_PATH    = 110
    *         TF_ERROR        = 111
    *         NOT_INITIALIZED = 112
    *         INVALID_PLUGIN  = 113
    *         INTERNAL_ERROR  = 114
    *         121..149 are reserved as plugin specific errors
    */
  uint32_t computeVelocityCommands(const geometry_msgs::PoseStamped& pose, const geometry_msgs::TwistStamped& velocity,
                                   geometry_msgs::TwistStamped &cmd_vel, std::string &message);

  /**
    * @brief  Check if the goal pose has been achieved
    * 
    * The actual check is performed in computeVelocityCommands(). 
    * Only the status flag is checked here.
    * @return True if achieved, false otherwise
    */
  bool isGoalReached();

  /**
    * @brief Dummy version to satisfy MBF API
    */
  bool isGoalReached(double xy_tolerance, double yaw_tolerance) { return isGoalReached(); };

  /**
    * @brief Requests the planner to cancel, e.g. if it takes too much time
    * @remark New on MBF API
    * @return True if a cancel has been successfully requested, false if not implemented.
    */
  bool cancel() { return false; };


  /** @name Public utility functions/methods */
  //@{
  
    /**
    * @brief  Transform a tf::Pose type into a Eigen::Vector2d containing the translational and angular velocities.
    * 
    * Translational velocities (x- and y-coordinates) are combined into a single translational velocity (first component).
    * @param tf_vel tf::Pose message containing a 1D or 2D translational velocity (x,y) and an angular velocity (yaw-angle)
    * @return Translational and angular velocity combined into an Eigen::Vector2d
    */
  static Eigen::Vector2d tfPoseToEigenVector2dTransRot(const tf::Pose& tf_vel);

  /**
   * @brief Get the current robot footprint/contour model
   * @param nh const reference to the local ros::NodeHandle
   * @param config const reference to the current configuration
   * @return Robot footprint model used for optimization
   */
  static RobotFootprintModelPtr getRobotFootprintFromParamServer(const ros::NodeHandle& nh, const TebConfig& config);
  
  /** 
   * @brief Set the footprint from the given XmlRpcValue.
   * @remarks This method is copied from costmap_2d/footprint.h, since it is not declared public in all ros distros
   * @remarks It is modified in order to return a container of Eigen::Vector2d instead of geometry_msgs::Point
   * @param footprint_xmlrpc should be an array of arrays, where the top-level array should have 3 or more elements, and the
   * sub-arrays should all have exactly 2 elements (x and y coordinates).
   * @param full_param_name this is the full name of the rosparam from which the footprint_xmlrpc value came. 
   * It is used only for reporting errors. 
   * @return container of vertices describing the polygon
   */
  static Point2dContainer makeFootprintFromXMLRPC(XmlRpc::XmlRpcValue& footprint_xmlrpc, const std::string& full_param_name);
  
  /** 
   * @brief Get a number from the given XmlRpcValue.
   * @remarks This method is copied from costmap_2d/footprint.h, since it is not declared public in all ros distros
   * @remarks It is modified in order to return a container of Eigen::Vector2d instead of geometry_msgs::Point
   * @param value double value type
   * @param full_param_name this is the full name of the rosparam from which the footprint_xmlrpc value came. 
   * It is used only for reporting errors. 
   * @returns double value
   */
  static double getNumberFromXMLRPC(XmlRpc::XmlRpcValue& value, const std::string& full_param_name);
  
  //@}
  
protected:

  /**
    * @brief Update internal obstacle vector based on occupied costmap cells
    * @remarks All occupied cells will be added as point obstacles.
    * @remarks All previous obstacles are cleared.
    * @sa updateObstacleContainerWithCostmapConverter
    * @todo Include temporal coherence among obstacle msgs (id vector)
    * @todo Include properties for dynamic obstacles (e.g. using constant velocity model)
    */
  void updateObstacleContainerWithCostmap();
  
  /**
   * @brief Update internal obstacle vector based on polygons provided by a costmap_converter plugin
   * @remarks Requires a loaded costmap_converter plugin.
   * @remarks All previous obstacles are cleared.
   * @sa updateObstacleContainerWithCostmap
   */
  void updateObstacleContainerWithCostmapConverter();
  
  /**
   * @brief Update internal obstacle vector based on custom messages received via subscriber
   * @remarks All previous obstacles are NOT cleared. Call this method after other update methods.
   * @sa updateObstacleContainerWithCostmap, updateObstacleContainerWithCostmapConverter
   */
  void updateObstacleContainerWithCustomObstacles();


  /**
   * @brief Update internal via-point container based on the current reference plan
   * @remarks All previous via-points will be cleared.
   * @param transformed_plan (local) portion of the global plan (which is already transformed to the planning frame)
   * @param min_separation minimum separation between two consecutive via-points
   */
  void updateViaPointsContainer(const std::vector<geometry_msgs::PoseStamped>& transformed_plan, double min_separation);
  
  
  /**
    * @brief Callback for the dynamic_reconfigure node.
    * 
    * This callback allows to modify parameters dynamically at runtime without restarting the node
    * @param config Reference to the dynamic reconfigure config
    * @param level Dynamic reconfigure level
    */
  void reconfigureCB(TebLocalPlannerReconfigureConfig& config, uint32_t level);
  
  
   /**
    * @brief Callback for custom obstacles that are not obtained from the costmap 
    * @param obst_msg pointer to the message containing a list of polygon shaped obstacles
    */
  void customObstacleCB(const costmap_converter::ObstacleArrayMsg::ConstPtr& obst_msg);
  
   /**
    * @brief Callback for custom via-points
    * @param via_points_msg pointer to the message containing a list of via-points
    */
  void customViaPointsCB(const nav_msgs::Path::ConstPtr& via_points_msg);

   /**
    * @brief Prune global plan such that already passed poses are cut off
    * 
    * The pose of the robot is transformed into the frame of the global plan by taking the most recent tf transform.
    * If no valid transformation can be found, the method returns \c false.
    * The global plan is pruned until the distance to the robot is at least \c dist_behind_robot.
    * If no pose within the specified treshold \c dist_behind_robot can be found,
    * nothing will be pruned and the method returns \c false.
    * @remarks Do not choose \c dist_behind_robot too small (not smaller the cellsize of the map), otherwise nothing will be pruned.
    * @param tf A reference to a tf buffer
    * @param global_pose The global pose of the robot
    * @param[in,out] global_plan The plan to be transformed
    * @param dist_behind_robot Distance behind the robot that should be kept [meters]
    * @return \c true if the plan is pruned, \c false in case of a transform exception or if no pose cannot be found inside the threshold
    */
  bool pruneGlobalPlan(const geometry_msgs::PoseStamped& global_pose,
                       std::vector<geometry_msgs::PoseStamped>& global_plan, double dist_behind_robot=1);
  
  /**
    * @brief  Transforms the global plan of the robot from the planner frame to the local frame (modified).
    * 
    * The method replaces transformGlobalPlan as defined in base_local_planner/goal_functions.h 
    * such that the index of the current goal pose is returned as well as 
    * the transformation between the global plan and the planning frame.
    * @param tf A reference to a tf buffer
    * @param global_plan The plan to be transformed
    * @param global_pose The global pose of the robot
    * @param costmap A reference to the costmap being used so the window size for transforming can be computed
    * @param global_frame The frame to transform the plan to
    * @param max_plan_length Specify maximum length (cumulative Euclidean distances) of the transformed plan [if <=0: disabled; the length is also bounded by the local costmap size!]
    * @param[out] transformed_plan Populated with the transformed plan
    * @param[out] current_goal_idx Index of the current (local) goal pose in the global plan
    * @param[out] tf_plan_to_global Transformation between the global plan and the global planning frame
    * @return \c true if the global plan is transformed, \c false otherwise
    */
  bool transformGlobalPlan(const std::vector<geometry_msgs::PoseStamped>& global_plan,
                           const geometry_msgs::PoseStamped& global_pose,  /*const costmap_2d::Costmap2D& costmap,*/
                           const std::string& global_frame, double max_plan_length, std::vector<geometry_msgs::PoseStamped>& transformed_plan,
                           int* current_goal_idx = NULL, geometry_msgs::TransformStamped* tf_plan_to_global = NULL);
    
  /**
    * @brief Estimate the orientation of a pose from the global_plan that is treated as a local goal for the local planner.
    * 
    * If the current (local) goal point is not the final one (global)
    * substitute the goal orientation by the angle of the direction vector between 
    * the local goal and the subsequent pose of the global plan. 
    * This is often helpful, if the global planner does not consider orientations. \n
    * A moving average filter is utilized to smooth the orientation.
    * @param global_plan The global plan
    * @param local_goal Current local goal
    * @param current_goal_idx Index of the current (local) goal pose in the global plan
    * @param[out] tf_plan_to_global Transformation between the global plan and the global planning frame
    * @param moving_average_length number of future poses of the global plan to be taken into account
    * @return orientation (yaw-angle) estimate
    */
  double estimateLocalGoalOrientation(const std::vector<geometry_msgs::PoseStamped>& global_plan, const geometry_msgs::PoseStamped& local_goal,
                                      int current_goal_idx, const geometry_msgs::TransformStamped& tf_plan_to_global, int moving_average_length=3) const;
        
        
  /**
   * @brief Saturate the translational and angular velocity to given limits.
   * 
   * The limit of the translational velocity for backwards driving can be changed independently.
   * Do not choose max_vel_x_backwards <= 0. If no backward driving is desired, change the optimization weight for
   * penalizing backwards driving instead.
   * @param[in,out] vx The translational velocity that should be saturated.
   * @param[in,out] vy Strafing velocity which can be nonzero for holonomic robots
   * @param[in,out] omega The angular velocity that should be saturated.
   * @param max_vel_x Maximum translational velocity for forward driving
   * @param max_vel_y Maximum strafing velocity (for holonomic robots)
   * @param max_vel_trans Maximum translational velocity for holonomic robots
   * @param max_vel_theta Maximum (absolute) angular velocity
   * @param max_vel_x_backwards Maximum translational velocity for backwards driving
   */
  void saturateVelocity(double& vx, double& vy, double& omega, double max_vel_x, double max_vel_y,
                        double max_vel_trans, double max_vel_theta, double max_vel_x_backwards) const;

  
  /**
   * @brief Convert translational and rotational velocities to a steering angle of a carlike robot
   * 
   * The conversion is based on the following equations:
   * - The turning radius is defined by \f$ R = v/omega \f$
   * - For a car like robot withe a distance L between both axles, the relation is: \f$ tan(\phi) = L/R \f$
   * - phi denotes the steering angle.
   * @remarks You might provide distances instead of velocities, since the temporal information is not required.
   * @param v translational velocity [m/s]
   * @param omega rotational velocity [rad/s]
   * @param wheelbase distance between both axles (drive shaft and steering axle), the value might be negative for back_wheeled robots
   * @param min_turning_radius Specify a lower bound on the turning radius
   * @return Resulting steering angle in [rad] inbetween [-pi/2, pi/2]
   */
  double convertTransRotVelToSteeringAngle(double v, double omega, double wheelbase, double min_turning_radius = 0) const;
  
  /**
   * @brief Validate current parameter values of the footprint for optimization, obstacle distance and the costmap footprint
   * 
   * This method prints warnings if validation fails.
   * @remarks Currently, we only validate the inscribed radius of the footprints
   * @param opt_inscribed_radius Inscribed radius of the RobotFootprintModel for optimization
   * @param costmap_inscribed_radius Inscribed radius of the footprint model used for the costmap
   * @param min_obst_dist desired distance to obstacles
   */
  void validateFootprints(double opt_inscribed_radius, double costmap_inscribed_radius, double min_obst_dist);
  
  
  void configureBackupModes(std::vector<geometry_msgs::PoseStamped>& transformed_plan,  int& goal_idx);


  void executeCallback(const trajectory_control_msgs::TrajectoryControlGoalConstPtr &goal_msg);
  void robotPathCallBack(const nav_msgs::PathConstPtr&);
  void robotLocalPathCallBack(const nav_msgs::PathConstPtr&);

  void queueFeedbackCallback(const trajectory_control_msgs::PlanningFeedback& feedback_msg);
  void queueTaskCallback(const nav_msgs::Path& path_msg);
  void queueLocalTaskCallback(const nav_msgs::Path& path_msg);
  
  void goalAbortCallback(const std_msgs::Bool& msg); 
  
  // void laserProximityCallback(const std_msgs::Bool& msg); 
  // void closestObstaclePointCallback(const std_msgs::Float32MultiArray& msg); 
  // void closestObstacleVelReductionEnableCallback(const std_msgs::Bool& msg);
  // void checkLaserProximityAndUpdateVelocity();

  void sendVelCmd(const geometry_msgs::Twist& cmd_twist);
  void getTracksVelCmd(const geometry_msgs::Twist& cmd_twist, nifti_robot_driver_msgs::Tracks& tracks_cmd);

  void clearOldObstacles(); 

private:
  // Definition of member variables

  // external objects (store weak pointers)
  //costmap_2d::Costmap2DROS* costmap_ros_; //!< Pointer to the costmap ros wrapper, received from the navigation stack
  //costmap_2d::Costmap2D* costmap_; //!< Pointer to the 2d costmap (obtained from the costmap ros wrapper)
  //tf2_ros::Buffer* tf_; //!< pointer to tf buffer
    
  // internal objects (memory management owned)
  PlannerInterfacePtr planner_; //!< Instance of the underlying optimal planner class
  ObstContainer obstacles_; //!< Obstacle vector that should be considered during local trajectory optimization
  std::vector<ros::Time> obstacles_stamps_; 
  ViaPointContainer via_points_; //!< Container of via-points that should be considered during local trajectory optimization
  TebVisualizationPtr visualization_; //!< Instance of the visualization class (local/global plan, obstacles, ...)
  //boost::shared_ptr<base_local_planner::CostmapModel> costmap_model_;  
  TebConfig cfg_; //!< Config class that stores and manages all related parameters
  FailureDetector failure_detector_; //!< Detect if the robot got stucked
  
  std::vector<geometry_msgs::PoseStamped> global_plan_; //!< Store the current global plan
  
  base_local_planner::OdometryHelperRos odom_helper_; //!< Provides an interface to receive the current velocity from the robot
  
  //pluginlib::ClassLoader<costmap_converter::BaseCostmapToPolygons> costmap_converter_loader_; //!< Load costmap converter plugins at runtime
  //boost::shared_ptr<costmap_converter::BaseCostmapToPolygons> costmap_converter_; //!< Store the current costmap_converter  

  boost::shared_ptr< dynamic_reconfigure::Server<TebLocalPlannerReconfigureConfig> > dynamic_recfg_; //!< Dynamic reconfigure server to allow config modifications at runtime
  ros::Subscriber custom_obst_sub_; //!< Subscriber for custom obstacles received via a ObstacleMsg.
  boost::mutex custom_obst_mutex_; //!< Mutex that locks the obstacle array (multi-threaded)
  costmap_converter::ObstacleArrayMsg custom_obstacle_msg_; //!< Copy of the most recent obstacle message

  ros::Subscriber via_points_sub_; //!< Subscriber for custom via-points received via a Path msg.
  bool custom_via_points_active_; //!< Keep track whether valid via-points have been received from via_points_sub_
  boost::mutex via_point_mutex_; //!< Mutex that locks the via_points container (multi-threaded)

  PoseSE2 robot_pose_; //!< Store current robot pose
  PoseSE2 robot_goal_; //!< Store current robot goal
  geometry_msgs::Twist robot_vel_; //!< Store current robot translational and angular velocity (vx, vy, omega)
  bool goal_reached_; //!< store whether the goal is reached or not
  ros::Time time_last_infeasible_plan_; //!< Store at which time stamp the last infeasible plan was detected
  int no_infeasible_plans_; //!< Store how many times in a row the planner failed to find a feasible plan.
  ros::Time time_last_oscillation_; //!< Store at which time stamp the last oscillation was detected
  RotType last_preferred_rotdir_; //!< Store recent preferred turning direction
  geometry_msgs::Twist last_cmd_; //!< Store the last control command generated in computeVelocityCommands()
  
  std::vector<geometry_msgs::Point> footprint_spec_; //!< Store the footprint of the robot 
  double robot_inscribed_radius_; //!< The radius of the inscribed circle of the robot (collision possible)
  double robot_circumscribed_radius; //!< The radius of the circumscribed circle of the robot
  
  std::string global_frame_; //!< The frame in which the controller will run
  std::string robot_base_frame_; //!< Used as the base frame id of the robot
  std::string name_; //!< For use with the ros nodehandle

  bool initialized_; //!< Keeps track about the correct initialization of this class


  ros::NodeHandle node_;
  ros::NodeHandle param_node_;

  std::string action_name;
  actionlib::SimpleActionServer<trajectory_control_msgs::TrajectoryControlAction> act_server_;
  actionlib::SimpleActionClient<trajectory_control_msgs::TrajectoryControlAction> act_client_;

  trajectory_control_msgs::TrajectoryControlFeedback feedback_msg_;
  trajectory_control_msgs::TrajectoryControlResult result_msg_;
  double control_rate_;

  tf::StampedTransform tf_robot_pose_;
  tf::TransformListener tf_listener_;
        
  geometry_msgs::Twist last_cmd_twist_;

  double transform_tolerance_ = 0.2; // [s] time tolerance for waiting a transform  
  bool use_proportional_saturation_ = false; // If true, reduce all twists components (linear x and y, and angular z) proportionally if any exceed its corresponding bounds, instead of saturating each one individually

  double tracks_distance_; // only used for transforming the twist into left-right velocity commands
  bool enable_track_cmds_ = false; 

  double factor_extending_plan_lookahead_dist_with_obs_ = 1.0; 
  double max_time_for_evanescent_obstacles_ = 1.0; // [s] 

protected: /// < queue controller stuff 
    
  ros::Publisher queue_task_feedback_pub_; // to say "hey, I'm ready!"
  ros::Subscriber queue_task_feedback_sub_; // to know when to stop
  ros::Subscriber queue_task_path_sub_; // path to follow (global path planning)
  ros::Subscriber queue_task_path_local_sub_; // path to follow (local path planning)
  
  std::string queue_task_feedback_topic_;
  std::string queue_task_path_topic_; 
  
  std::string queue_task_path_local_topic_; 
  
  std::atomic<bool> b_local_path_;
  std::atomic<bool> b_simple_rotation_;

  nav_msgs::Path global_path_msg_;
  nav_msgs::Path local_path_msg_;
  nav_msgs::Path global_plan_msg_;

  std::string global_path_topic_;
  ros::Publisher global_path_pub_;

  std::string local_path_topic_;
  ros::Publisher local_path_pub_;

  std::string cmd_topic_;
  ros::Publisher cmd_pub_;

  std::string cmd_wheels_topic_;
  ros::Publisher cmd_wheels_pub_;

  std::string robot_path_topic_;
  ros::Subscriber robot_path_sub_;
  
  std::string robot_local_path_topic_;
  ros::Subscriber robot_loal_path_sub_;

  std::string goal_abort_topic_;
  ros::Subscriber goal_abort_sub_;
  
  std::string trajectory_control_abort_topic_;
  ros::Subscriber trajectory_control_abort_sub_;

  std::string tracks_vel_cmd_topic_;
  ros::Publisher tracks_vel_cmd_pub_;    
    
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
};
  
}; // end namespace teb_local_planner


