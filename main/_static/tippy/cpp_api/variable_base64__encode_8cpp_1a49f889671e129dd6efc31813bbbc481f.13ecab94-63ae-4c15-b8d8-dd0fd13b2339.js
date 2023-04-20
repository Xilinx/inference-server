selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_models_base64_encode.cpp.html#file-workspace-amdinfer-src-amdinfer-models-base64-encode-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File base64_encode.cpp<a class=\"headerlink\" href=\"#file-base64-encode-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_models.html#dir-workspace-amdinfer-src-amdinfer-models\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/models</span></code>)</p><p>Implements the base64_codec model.</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#variable-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#variable-kmaximagechannels\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Variable kMaxImageChannels<a class=\"headerlink\" href=\"#variable-kmaximagechannels\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
