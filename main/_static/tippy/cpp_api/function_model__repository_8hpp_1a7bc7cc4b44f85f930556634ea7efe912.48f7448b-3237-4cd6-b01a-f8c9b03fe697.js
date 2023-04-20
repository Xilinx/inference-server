selector_to_html = {"a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-amdinfer-parsemodel-const-std-filesystem-path-const-std-string\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::parseModel(const std::filesystem::path&amp;, const std::string&amp;)<a class=\"headerlink\" href=\"#function-amdinfer-parsemodel-const-std-filesystem-path-const-std-string\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer10parseModelERKNSt10filesystem4pathERKNSt6stringE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer10parseModelERKNSt10filesystem4pathERKNSt6stringE\">\n<span id=\"_CPPv3N8amdinfer10parseModelERKNSt10filesystem4pathERKNSt6stringE\"></span><span id=\"_CPPv2N8amdinfer10parseModelERKNSt10filesystem4pathERKNSt6stringE\"></span><span id=\"amdinfer::parseModel__std::filesystem::pathCR.ssCR\"></span><span class=\"target\" id=\"model__repository_8hpp_1a7bc7cc4b44f85f930556634ea7efe912\"></span><a class=\"reference internal\" href=\"classamdinfer_1_1ModelConfig.html#_CPPv4N8amdinfer11ModelConfigE\" title=\"amdinfer::ModelConfig\"><span class=\"n\"><span class=\"pre\">ModelConfig</span></span></a><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">parseModel</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">filesystem</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">path</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">repository</span></span>, <span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">model</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_model_repository.hpp.html#file-workspace-amdinfer-src-amdinfer-core-model-repository-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File model_repository.hpp<a class=\"headerlink\" href=\"#file-model-repository-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>", "a[href=\"classamdinfer_1_1ModelConfig.html#_CPPv4N8amdinfer11ModelConfigE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11ModelConfigE\">\n<span id=\"_CPPv3N8amdinfer11ModelConfigE\"></span><span id=\"_CPPv2N8amdinfer11ModelConfigE\"></span><span id=\"amdinfer::ModelConfig\"></span><span class=\"target\" id=\"classamdinfer_1_1ModelConfig\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">ModelConfig</span></span></span><br/></dt><dd></dd>"}
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
