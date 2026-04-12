from setuptools import setup


package_name = "rscf_a_host"


setup(
    name=package_name,
    version="0.1.0",
    packages=[package_name],
    data_files=[
        ("share/ament_index/resource_index/packages", ["resource/" + package_name]),
        ("share/" + package_name, ["package.xml"]),
        ("share/" + package_name + "/launch", ["launch/rscf_host_io.launch.py"]),
        ("share/" + package_name + "/config", ["config/command_profile.yaml"]),
        ("share/" + package_name + "/scripts", ["scripts/run_agent_usb.sh"]),
    ],
    install_requires=["setuptools"],
    zip_safe=True,
    maintainer="xuan",
    maintainer_email="xuan@example.com",
    description="Host-side ROS 2 tools for RSCF A micro-ROS communication.",
    license="Apache-2.0",
    entry_points={
        "console_scripts": [
            "rscf_monitor = rscf_a_host.monitor_node:main",
            "rscf_command_profile = rscf_a_host.command_profile_node:main",
        ],
    },
)
