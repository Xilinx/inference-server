selector_to_html = {"a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer24extractModelConfigTensorERKN4toml5tableE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer24extractModelConfigTensorERKN4toml5tableE\">\n<span id=\"_CPPv3N8amdinfer24extractModelConfigTensorERKN4toml5tableE\"></span><span id=\"_CPPv2N8amdinfer24extractModelConfigTensorERKN4toml5tableE\"></span><span id=\"amdinfer::extractModelConfigTensor__toml::tableCR\"></span><span class=\"target\" id=\"model__config_8cpp_1a8760611469261389e290e73112952772\"></span><a class=\"reference internal\" href=\"classamdinfer_1_1ModelConfigTensor.html#_CPPv4N8amdinfer17ModelConfigTensorE\" title=\"amdinfer::ModelConfigTensor\"><span class=\"n\"><span class=\"pre\">ModelConfigTensor</span></span></a><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">extractModelConfigTensor</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">toml</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">table</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">table</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_model_config.cpp.html#file-workspace-amdinfer-src-amdinfer-core-model-config-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File model_config.cpp<a class=\"headerlink\" href=\"#file-model-config-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>", "a[href=\"#function-amdinfer-extractmodelconfigtensor\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::extractModelConfigTensor<a class=\"headerlink\" href=\"#function-amdinfer-extractmodelconfigtensor\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classamdinfer_1_1ModelConfigTensor.html#_CPPv4N8amdinfer17ModelConfigTensorE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer17ModelConfigTensorE\">\n<span id=\"_CPPv3N8amdinfer17ModelConfigTensorE\"></span><span id=\"_CPPv2N8amdinfer17ModelConfigTensorE\"></span><span id=\"amdinfer::ModelConfigTensor\"></span><span class=\"target\" id=\"classamdinfer_1_1ModelConfigTensor\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">ModelConfigTensor</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">:</span></span><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">public</span></span><span class=\"w\"> </span><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><a class=\"reference internal\" href=\"classamdinfer_1_1Tensor.html#_CPPv4N8amdinfer6TensorE\" title=\"amdinfer::Tensor\"><span class=\"n\"><span class=\"pre\">Tensor</span></span></a><br/></dt><dd></dd>"}
skip_classes = ["headerlink", "sd-stretched-link"]

window.onload = function () {
    for (const [select, tip_html] of Object.entries(selector_to_html)) {
        const links = document.querySelectorAll(` ${select}`);
        for (const link of links) {
            if (skip_classes.some(c => link.classList.contains(c))) {
                continue;
            }

            tippy(link, {
                content: tip_html,
                allowHTML: true,
                arrow: true,
                placement: 'auto-start', maxWidth: 500, interactive: false,

            });
        };
    };
    console.log("tippy tips loaded!");
};
