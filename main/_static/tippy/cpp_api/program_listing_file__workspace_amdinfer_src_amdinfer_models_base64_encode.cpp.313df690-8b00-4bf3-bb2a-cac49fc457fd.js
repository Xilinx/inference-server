selector_to_html = {"a[href=\"#program-listing-for-file-base64-encode-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Program Listing for File base64_encode.cpp<a class=\"headerlink\" href=\"#program-listing-for-file-base64-encode-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"file__workspace_amdinfer_src_amdinfer_models_base64_encode.cpp.html#file-workspace-amdinfer-src-amdinfer-models-base64-encode-cpp\"><span class=\"std std-ref\">Return to documentation for file</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/models/base64_encode.cpp</span></code>)</p>", "a[href=\"file__workspace_amdinfer_src_amdinfer_models_base64_encode.cpp.html#file-workspace-amdinfer-src-amdinfer-models-base64-encode-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File base64_encode.cpp<a class=\"headerlink\" href=\"#file-base64-encode-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_models.html#dir-workspace-amdinfer-src-amdinfer-models\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/models</span></code>)</p><p>Implements the base64_codec model.</p>"}
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
