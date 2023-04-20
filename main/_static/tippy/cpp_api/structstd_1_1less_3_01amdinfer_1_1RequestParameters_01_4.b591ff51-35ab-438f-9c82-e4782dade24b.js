selector_to_html = {"a[href=\"#template-struct-less-amdinfer-requestparameters\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Struct less&lt; amdinfer::RequestParameters &gt;<a class=\"headerlink\" href=\"#template-struct-less-amdinfer-requestparameters\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_include_amdinfer_core_predict_api.hpp.html#file-workspace-amdinfer-include-amdinfer-core-predict-api-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File predict_api.hpp<a class=\"headerlink\" href=\"#file-predict-api-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer_core.html#dir-workspace-amdinfer-include-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer/core</span></code>)</p><p>Defines the objects used to hold inference requests and responses.</p>", "a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
