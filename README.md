# MCAP_Editor

A GUI to edit your [MCAP files](https://mcap.dev/). Currently tested on Qt 6.6.

You can try it online here: https://mcap-editor.netlify.app/

![screenshot](screenshot.png)


## Installation

- Clone the repository e.g. to `~/ros2_ws/src`
- Install Qt 6
    ```bash
    sudo apt install qt6-default
    ```
- Colcon build
    ```bash
    cd ~/ros2_ws
    ```
    ```bash
    colcon build --symlink-install --packages-select mcap_editor
    ```

## Usage

<details>
<summary> Don't forget to source before ROS commands.</summary>

``` bash
source ~/ros2_ws/install/setup.bash
```
</details>

``` bash
ros2 run mcap_editor mcap_editor
```