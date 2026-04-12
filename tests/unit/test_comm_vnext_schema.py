from pathlib import Path
import importlib.util
import json
import yaml


REPO_ROOT = Path(__file__).resolve().parents[2]
GENERATOR_DIR = REPO_ROOT / "mcu_comm" / "protocol_generator"
PROTOCOL_PATH = GENERATOR_DIR / "protocol_definition.json"
ENDPOINT_SCHEMA_PATH = GENERATOR_DIR / "endpoint_schema.yaml"
LINK_ROLES_PATH = GENERATOR_DIR / "link_roles.yaml"
CODEGEN_SCRIPT = REPO_ROOT / "scripts" / "codegen" / "rscf_mcu_comm_codegen.py"


def _load_codegen_module():
    spec = importlib.util.spec_from_file_location("rscf_mcu_comm_codegen", CODEGEN_SCRIPT)
    module = importlib.util.module_from_spec(spec)
    assert spec is not None
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


def test_vnext_schema_files_present_and_loadable():
    protocol = json.loads(PROTOCOL_PATH.read_text(encoding="utf-8"))

    assert "endpoints" in protocol
    assert "links" in protocol

    endpoints = protocol["endpoints"]
    links = protocol["links"]

    assert endpoints.get("schema") == "endpoint_schema.yaml"
    assert links.get("roles") == "link_roles.yaml"
    assert "compat" in endpoints
    assert "by_mid" in endpoints["compat"]
    assert "by_device" in endpoints["compat"]

    endpoint_schema = yaml.safe_load(ENDPOINT_SCHEMA_PATH.read_text(encoding="utf-8"))
    link_roles = yaml.safe_load(LINK_ROLES_PATH.read_text(encoding="utf-8"))

    assert endpoint_schema.get("meta", {}).get("schema")
    assert endpoint_schema.get("fields")
    assert link_roles.get("meta", {}).get("schema")
    assert link_roles.get("roles")


def test_codegen_context_exposes_vnext_metadata():
    module = _load_codegen_module()
    generator = module.ZephyrProtocolGenerator(str(module.PROTOCOL_JSON), str(REPO_ROOT))
    assert generator.load_protocol()

    context = generator.build_zephyr_context("MCU_ONE")

    assert "vnext" in context
    assert "vnext_endpoints" in context
    assert "vnext_links" in context
    assert "vnext_endpoint_schema" in context
    assert "vnext_link_roles" in context
    assert "vnext_compat_endpoints" in context
    assert context["vnext_compat_endpoints"]["by_mid"]["0x05"] == "mcu_one"


def test_base_generator_loads_vnext_metadata_without_breaking_contract():
    module = _load_codegen_module()
    generator = module.load_generator_class()(str(module.PROTOCOL_JSON), str(REPO_ROOT))

    assert generator.load_protocol()
    assert generator.protocol_data["endpoint_schema"]["meta"]["schema"] == "rscf-link-vnext-endpoint"
    assert generator.protocol_data["link_roles"]["meta"]["schema"] == "rscf-link-vnext-roles"
    assert generator.protocol_data["compat_endpoints"]["by_device"]["MCU_ONE"] == "mcu_one"

    vnext_context = generator._build_vnext_context()

    assert vnext_context["vnext"]["endpoints"]["schema"] == "endpoint_schema.yaml"
    assert vnext_context["vnext_endpoints"]["compat"]["by_mid"]["0x05"] == "mcu_one"
    assert vnext_context["vnext_links"]["roles"] == "link_roles.yaml"
