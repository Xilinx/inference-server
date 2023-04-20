selector_to_html = {"a[href=\"#c.AMDINFER_IF_LOGGING\"]": "<dt class=\"sig sig-object cpp\" id=\"c.AMDINFER_IF_LOGGING\">\n<span class=\"target\" id=\"logging_8hpp_1aa0607a02582d3aef0021738df1b73c93\"></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">AMDINFER_IF_LOGGING</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">args</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"#define-amdinfer-if-logging\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Define AMDINFER_IF_LOGGING<a class=\"headerlink\" href=\"#define-amdinfer-if-logging\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#define-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_observation_logging.hpp.html#file-workspace-amdinfer-src-amdinfer-observation-logging-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File logging.hpp<a class=\"headerlink\" href=\"#file-logging-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_observation.html#dir-workspace-amdinfer-src-amdinfer-observation\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation</span></code>)</p><p>Defines logging.</p>"}
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
