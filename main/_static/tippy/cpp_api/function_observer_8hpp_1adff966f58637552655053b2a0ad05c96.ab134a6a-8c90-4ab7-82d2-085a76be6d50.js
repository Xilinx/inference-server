selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_observation_observer.hpp.html#file-workspace-amdinfer-src-amdinfer-observation-observer-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File observer.hpp<a class=\"headerlink\" href=\"#file-observer-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_observation.html#dir-workspace-amdinfer-src-amdinfer-observation\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation</span></code>)</p>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-amdinfer-logtracebuffer\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::logTraceBuffer<a class=\"headerlink\" href=\"#function-amdinfer-logtracebuffer\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
