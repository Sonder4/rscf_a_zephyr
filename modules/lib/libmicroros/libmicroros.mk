SHELL := /bin/bash

UROS_DIR = $(COMPONENT_PATH)/micro_ros_src
DEBUG ?= 0
MICROROS_LOCAL_MCU_WS ?=
MICROROS_PYTHON ?= /usr/bin/python3
MICROROS_DEV_PACKAGES ?= ament_cmake_ros ament_index_cpp ament_index_python ament_package

ifeq ($(DEBUG), 1)
	BUILD_TYPE = Debug
else
	BUILD_TYPE = Release
endif

CFLAGS_INTERNAL := $(X_CFLAGS)
CXXFLAGS_INTERNAL := $(X_CXXFLAGS)

CFLAGS_INTERNAL := -c -include strings.h \
	-I$(ZEPHYR_BASE)/modules/cmsis_6 \
	-I$(ZEPHYR_BASE)/modules/cmsis \
	-I$(dir $(ZEPHYR_BASE))/third_party/hal/cmsis/CMSIS/Core/Include \
	-I$(ZEPHYR_BASE)/include/posix \
	-I$(PROJECT_BINARY_DIR)/include/generated \
	$(CFLAGS_INTERNAL)
CXXFLAGS_INTERNAL := -c \
	-I$(ZEPHYR_BASE)/modules/cmsis_6 \
	-I$(ZEPHYR_BASE)/modules/cmsis \
	-I$(dir $(ZEPHYR_BASE))/third_party/hal/cmsis/CMSIS/Core/Include \
	-I$(ZEPHYR_BASE)/include/posix \
	-I$(PROJECT_BINARY_DIR)/include/generated \
	$(CXXFLAGS_INTERNAL)

all: $(COMPONENT_PATH)/libmicroros.a

clean:
	rm -rf $(COMPONENT_PATH)/libmicroros.a; \
	rm -rf $(COMPONENT_PATH)/include; \
	rm -rf $(COMPONENT_PATH)/zephyr_toolchain.cmake; \
	rm -rf $(COMPONENT_PATH)/micro_ros_dev; \
	rm -rf $(COMPONENT_PATH)/micro_ros_src;

ZEPHYR_CONF_FILE := $(PROJECT_BINARY_DIR)/.config

get_package_names: $(COMPONENT_PATH)/micro_ros_src/src
	@cd $(COMPONENT_PATH)/micro_ros_src/src; \
	colcon list | awk '{print $$1}' | awk -v d=";" '{s=(NR==1?s:s d)$$0}END{print s}'

patch_vendor_sources: $(COMPONENT_PATH)/micro_ros_src/src
	bash $(COMPONENT_PATH)/patch_microros_sources.sh $(COMPONENT_PATH)

configure_colcon_meta: $(COMPONENT_PATH)/colcon.meta $(COMPONENT_PATH)/micro_ros_src/src
	. $(COMPONENT_PATH)/utils.sh; \
	cp $(COMPONENT_PATH)/colcon.meta $(COMPONENT_PATH)/configured_colcon.meta; \
	ZEPHYR_CONF_FILE=$(ZEPHYR_CONF_FILE); \
	update_meta_from_zephyr_config "CONFIG_MICROROS_NODES" "rmw_microxrcedds" "RMW_UXRCE_MAX_NODES"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_PUBLISHERS" "rmw_microxrcedds" "RMW_UXRCE_MAX_PUBLISHERS"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_SUBSCRIBERS" "rmw_microxrcedds" "RMW_UXRCE_MAX_SUBSCRIPTIONS"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_CLIENTS" "rmw_microxrcedds" "RMW_UXRCE_MAX_CLIENTS"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_SERVERS" "rmw_microxrcedds" "RMW_UXRCE_MAX_SERVICES"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_RMW_HISTORIC" "rmw_microxrcedds" "RMW_UXRCE_MAX_HISTORY"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_XRCE_DDS_HISTORIC" "rmw_microxrcedds" "RMW_UXRCE_STREAM_HISTORY"; \
	update_meta_from_zephyr_config "CONFIG_MICROROS_XRCE_DDS_MTU" "microxrcedds_client" "UCLIENT_CUSTOM_TRANSPORT_MTU"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_SERIAL=OFF"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_UDP=OFF"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_TCP=OFF"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_CUSTOM_TRANSPORT=ON"; \
	update_meta "microxrcedds_client" "UCLIENT_PROFILE_STREAM_FRAMING=ON"; \
	update_meta "rmw_microxrcedds" "RMW_UXRCE_TRANSPORT=custom";


configure_toolchain: $(COMPONENT_PATH)/zephyr_toolchain.cmake.in
	rm -f $(COMPONENT_PATH)/zephyr_toolchain.cmake; \
	cat $(COMPONENT_PATH)/zephyr_toolchain.cmake.in | \
		sed "s/@CMAKE_C_COMPILER@/$(subst /,\/,$(X_CC))/g" | \
		sed "s/@CMAKE_CXX_COMPILER@/$(subst /,\/,$(X_CXX))/g" | \
		sed "s/@CMAKE_SYSROOT@/$(subst /,\/,$(COMPONENT_PATH))/g" | \
		sed "s/@CFLAGS@/$(subst /,\/,$(CFLAGS_INTERNAL))/g" | \
		sed "s/@CXXFLAGS@/$(subst /,\/,$(CXXFLAGS_INTERNAL))/g" \
		> $(COMPONENT_PATH)/zephyr_toolchain.cmake

$(COMPONENT_PATH)/micro_ros_dev/install:
	rm -rf micro_ros_dev; \
	mkdir micro_ros_dev; cd micro_ros_dev; \
	git clone -b jazzy https://github.com/ament/ament_cmake src/ament_cmake; \
	git clone -b jazzy https://github.com/ament/ament_lint src/ament_lint; \
	git clone -b jazzy https://github.com/ament/ament_package src/ament_package; \
	git clone -b jazzy https://github.com/ament/googletest src/googletest; \
	git clone -b jazzy https://github.com/ros2/ament_cmake_ros src/ament_cmake_ros; \
	git clone -b jazzy https://github.com/ament/ament_index src/ament_index; \
	MICROROS_STRIP_PREFIXES="$${AMENT_PREFIX_PATH:-}:$${COLCON_PREFIX_PATH:-}"; \
	export MICROROS_STRIP_PREFIXES; \
	. $(COMPONENT_PATH)/sanitize_microros_env.sh; \
	unset MICROROS_STRIP_PREFIXES; \
	unset AMENT_PREFIX_PATH COLCON_PREFIX_PATH; \
	COLCON_PYTHON_EXECUTABLE=$(MICROROS_PYTHON) \
	PYTHONNOUSERSITE=1 \
	colcon build \
		--packages-up-to $(MICROROS_DEV_PACKAGES) \
		--cmake-clean-cache \
		--cmake-args \
		-DBUILD_TESTING=OFF \
		-DPython3_EXECUTABLE=$(MICROROS_PYTHON) \
		-DPYTHON_EXECUTABLE=$(MICROROS_PYTHON) \
		-DPython3_FIND_VIRTUALENV=STANDARD \
		-DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
		-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON \
		-DCMAKE_IGNORE_PREFIX_PATH=/opt/ros/humble\;/opt/ros/humble/local;

$(COMPONENT_PATH)/micro_ros_src/src:
	@rm -rf micro_ros_src; \
	mkdir micro_ros_src; cd micro_ros_src; \
	mkdir -p src; cd src; \
	link_or_clone() { \
		src_path="$$1"; \
		dst_path="$$2"; \
		branch="$$3"; \
		repo="$$4"; \
		if [ -n "$(MICROROS_LOCAL_MCU_WS)" ] && [ -e "$$src_path" ]; then \
			ln -s "$$src_path" "$$dst_path"; \
		else \
			git clone -b "$$branch" "$$repo" "$$dst_path"; \
		fi; \
	}; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/eProsima/Micro-CDR" micro-CDR ros2 https://github.com/eProsima/micro-CDR; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/eProsima/Micro-XRCE-DDS-Client" Micro-XRCE-DDS-Client ros2 https://github.com/eProsima/Micro-XRCE-DDS-Client; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/rcl" rcl jazzy https://github.com/micro-ROS/rcl; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/rclc" rclc jazzy https://github.com/ros2/rclc; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/rcutils" rcutils jazzy https://github.com/micro-ROS/rcutils; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/micro_ros_msgs" micro_ros_msgs jazzy https://github.com/micro-ROS/micro_ros_msgs; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/rmw_microxrcedds" rmw-microxrcedds jazzy https://github.com/micro-ROS/rmw-microxrcedds; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/rosidl_typesupport" rosidl_typesupport jazzy https://github.com/micro-ROS/rosidl_typesupport; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/rosidl_typesupport_microxrcedds" rosidl_typesupport_microxrcedds jazzy https://github.com/micro-ROS/rosidl_typesupport_microxrcedds; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rosidl" rosidl jazzy https://github.com/ros2/rosidl; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rmw" rmw jazzy https://github.com/ros2/rmw; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rcl_interfaces" rcl_interfaces jazzy https://github.com/ros2/rcl_interfaces; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rosidl_defaults" rosidl_defaults jazzy https://github.com/ros2/rosidl_defaults; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/unique_identifier_msgs" unique_identifier_msgs jazzy https://github.com/ros2/unique_identifier_msgs; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/common_interfaces" common_interfaces jazzy https://github.com/ros2/common_interfaces; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/test_interface_files" test_interface_files jazzy https://github.com/ros2/test_interface_files; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rmw_implementation" rmw_implementation jazzy https://github.com/ros2/rmw_implementation; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/rcl_logging" rcl_logging jazzy https://github.com/ros2/rcl_logging; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/ros2/ros2_tracing" ros2_tracing jazzy https://github.com/ros2/ros2_tracing; \
	link_or_clone "$(MICROROS_LOCAL_MCU_WS)/uros/micro_ros_utilities" micro_ros_utilities jazzy https://github.com/micro-ROS/micro_ros_utilities; \
	touch ros2_tracing/test_tracetools/COLCON_IGNORE 2>/dev/null || true; \
	touch ros2_tracing/lttngpy/COLCON_IGNORE 2>/dev/null || true; \
	touch rosidl_dynamic_typesupport/COLCON_IGNORE 2>/dev/null || true; \
    touch rosidl/rosidl_typesupport_introspection_cpp/COLCON_IGNORE 2>/dev/null || true; \
    touch rclc/rclc_examples/COLCON_IGNORE 2>/dev/null || true; \
    touch common_interfaces/actionlib_msgs/COLCON_IGNORE 2>/dev/null || true; \
	touch common_interfaces/std_srvs/COLCON_IGNORE 2>/dev/null || true; \
	touch rcl/rcl_yaml_param_parser/COLCON_IGNORE 2>/dev/null || true; \
    touch rcl_logging/rcl_logging_spdlog/COLCON_IGNORE 2>/dev/null || true; \
    touch rcl_interfaces/test_msgs/COLCON_IGNORE 2>/dev/null || true;

$(COMPONENT_PATH)/micro_ros_src/install: patch_vendor_sources configure_colcon_meta configure_toolchain $(COMPONENT_PATH)/micro_ros_dev/install $(COMPONENT_PATH)/micro_ros_src/src
	cd $(UROS_DIR); \
	MICROROS_STRIP_PREFIXES="$${AMENT_PREFIX_PATH:-}:$${COLCON_PREFIX_PATH:-}"; \
	export MICROROS_STRIP_PREFIXES; \
	. $(COMPONENT_PATH)/sanitize_microros_env.sh; \
	unset MICROROS_STRIP_PREFIXES; \
	unset AMENT_PREFIX_PATH COLCON_PREFIX_PATH; \
	. ../micro_ros_dev/install/local_setup.sh; \
	. $(COMPONENT_PATH)/sanitize_microros_env.sh; \
	COLCON_PYTHON_EXECUTABLE=$(MICROROS_PYTHON) \
	PYTHONNOUSERSITE=1 \
	colcon build \
		--merge-install \
		--packages-ignore-regex=.*_cpp \
		--cmake-clean-cache \
		--metas $(COMPONENT_PATH)/configured_colcon.meta \
		--cmake-args \
		"--no-warn-unused-cli" \
		-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=OFF \
		-DTHIRDPARTY=ON \
		-DBUILD_SHARED_LIBS=OFF \
		-DBUILD_TESTING=OFF \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_TOOLCHAIN_FILE=$(COMPONENT_PATH)/zephyr_toolchain.cmake \
		-DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE \
		-DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON \
		-DCMAKE_IGNORE_PREFIX_PATH=/opt/ros/humble\;/opt/ros/humble/local \
		-DPython3_FIND_VIRTUALENV=STANDARD \
		-DPython3_EXECUTABLE=$(MICROROS_PYTHON) \
		-DPYTHON_EXECUTABLE=$(MICROROS_PYTHON) \
		-DCMAKE_VERBOSE_MAKEFILE=OFF; \

$(COMPONENT_PATH)/libmicroros.a: $(COMPONENT_PATH)/micro_ros_src/install
	mkdir -p $(UROS_DIR)/libmicroros; cd $(UROS_DIR)/libmicroros; \
	for file in $$(find $(UROS_DIR)/install/lib/ -name '*.a'); do \
		folder=$$(echo $$file | sed -E "s/(.+)\/(.+).a/\2/"); \
		mkdir -p $$folder; cd $$folder; $(X_AR) x $$file; \
		for f in *; do \
			mv $$f ../$$folder-$$f; \
		done; \
		cd ..; rm -rf $$folder; \
	done ; \
	$(X_AR) rc libmicroros.a *.obj; cp libmicroros.a $(COMPONENT_PATH); ${X_RANLIB} $(COMPONENT_PATH)/libmicroros.a; \
	cd ..; rm -rf libmicroros; \
	rm -rf $(COMPONENT_PATH)/include; \
	cp -R $(UROS_DIR)/install/include $(COMPONENT_PATH)/include; \
	for pkg_dir in $(COMPONENT_PATH)/include/*; do \
		if [ -d "$$pkg_dir" ]; then \
			pkg_name=$$(basename "$$pkg_dir"); \
			if [ -d "$$pkg_dir/$$pkg_name" ]; then \
				cp -R "$$pkg_dir/$$pkg_name"/. "$$pkg_dir"/; \
				rm -rf "$$pkg_dir/$$pkg_name"; \
			fi; \
		fi; \
	done;
