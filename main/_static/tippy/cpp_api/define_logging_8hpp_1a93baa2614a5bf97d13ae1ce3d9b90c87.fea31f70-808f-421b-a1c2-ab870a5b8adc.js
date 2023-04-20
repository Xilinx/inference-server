selector_to_html = {"a[href=\"#c.AMDINFER_LOG_TRACE\"]": "<dt class=\"sig sig-object cpp\" id=\"c.AMDINFER_LOG_TRACE\">\n<span class=\"target\" id=\"logging_8hpp_1a93baa2614a5bf97d13ae1ce3d9b90c87\"></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">AMDINFER_LOG_TRACE</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">logger</span></span>, <span class=\"n\"><span class=\"pre\">message</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"#define-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_observation_logging.hpp.html#file-workspace-amdinfer-src-amdinfer-observation-logging-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File logging.hpp<a class=\"headerlink\" href=\"#file-logging-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_observation.html#dir-workspace-amdinfer-src-amdinfer-observation\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation</span></code>)</p><p>Defines logging.</p>", "a[href=\"#define-amdinfer-log-trace\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Define AMDINFER_LOG_TRACE<a class=\"headerlink\" href=\"#define-amdinfer-log-trace\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
