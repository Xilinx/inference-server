selector_to_html = {"a[href=\"#define-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_base64.cpp.html#file-workspace-amdinfer-src-amdinfer-util-base64-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File base64.cpp<a class=\"headerlink\" href=\"#file-base64-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p><p>Implements base64 encoding/decoding.</p>", "a[href=\"#define-buffersize\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Define BUFFERSIZE<a class=\"headerlink\" href=\"#define-buffersize\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
