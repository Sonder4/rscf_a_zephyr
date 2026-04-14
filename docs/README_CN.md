# RSCF A Zephyr 文档索引

当前仓库的默认通信主线已经切换为：

- MCU 侧：`RSCF Link vNext`
- Host 侧：`ros2_ws/src/rscf_link_bridge`
- 物理主链路：`USB CDC ACM`
- `micro-ROS`：兼容后端，默认关闭

推荐按下面顺序阅读：

1. [README.md](../README.md)
   说明仓库目标、默认通信方案、快速开始和当前迁移状态。
2. [getting-started.md](getting-started.md)
   说明环境准备、bootstrap、west 工作区和基础构建流程。
3. [zephyr-user-manual.md](zephyr-user-manual.md)
   说明如何加载驱动、如何写任务逻辑、如何新增驱动模板。
4. [reference/rscf_a_hardware_map.md](reference/rscf_a_hardware_map.md)
   说明板级硬件映射与 Zephyr 节点对应关系。
5. [ioc_json](ioc_json)
   保留旧工程 `.ioc` 解析后的硬件事实快照。

如果你现在要继续扩展通信链路，建议优先看：

- `modules/rscf/services/rscf_link_service.*`
- `drivers/rscf/rscf_link_*.c`
- `generated/mcu_comm/MCU_ONE/`
- `ros2_ws/src/rscf_link_bridge/`
