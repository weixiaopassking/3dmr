#!/usr/bin/env bash

if [[ -z "${MR3D_HOME}" ]]; then
    echo "ERROR: missing env var MR3D_HOME"
    echo "please, source source_all.bash in the main 3DMR folder"
    exit 
else
    source $MR3D_HOME/source_all.bash
fi

# this input_path.csv can be generated by running the script src/nav/trajectory_control/scripts/path_smoother_test.py
INPUT_FILE="$(rospack find trajectory_control)/scripts/input_path.csv"
echo input file: $INPUT_FILE

if [ ! -f "$INPUT_FILE" ]; then 
    echo missing input file $INPUT_FILE
    echo use src/nav/trajectory_control/scripts/path_smoother_test.py to generate the input file
    exit 1 
fi 

MODE=2 # int 
# kNoSmoother = 0, 
# kSmoother3 = 1,
# kSmoother5 = 2,
# kSmoother3InPlace = 3,
# kSmootherSGw5 = 4,
# kSmootherSGw7 = 5,
# kNumOfSmoothers,

rosrun trajectory_control test_smoother $INPUT_FILE $MODE
#rosrun --prefix 'gdb -ex run --args' trajectory_control test_smoother $INPUT_FILE $MODE   # for debugging with gdb 

DIRECTORY_FILE=$(dirname $INPUT_FILE)
rosrun trajectory_control draw_csv_path.py --input-file $DIRECTORY_FILE/output_path.csv