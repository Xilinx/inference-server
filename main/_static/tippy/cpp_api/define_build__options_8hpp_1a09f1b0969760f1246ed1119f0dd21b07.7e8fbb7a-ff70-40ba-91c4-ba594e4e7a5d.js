selector_to_html = {"a[href=\"#c.AMDINFER_ENABLE_MIGRAPHX\"]": "<dt class=\"sig sig-object cpp\" id=\"c.AMDINFER_ENABLE_MIGRAPHX\">\n<span class=\"target\" id=\"build__options_8hpp_1a09f1b0969760f1246ed1119f0dd21b07\"></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">AMDINFER_ENABLE_MIGRAPHX</span></span></span><br/></dt><dd><p>Enables MIGraphXr. </p></dd>", "a[href=\"#define-amdinfer-enable-migraphx\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Define AMDINFER_ENABLE_MIGRAPHX<a class=\"headerlink\" href=\"#define-amdinfer-enable-migraphx\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_include_amdinfer_build_options.hpp.html#file-workspace-amdinfer-include-amdinfer-build-options-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File build_options.hpp<a class=\"headerlink\" href=\"#file-build-options-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer.html#dir-workspace-amdinfer-include-amdinfer\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer</span></code>)</p><p>Defines the build information. This file is updated automatically by CMake. To update, recompile the amdinfer source code.</p>", "a[href=\"#define-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Define Documentation<a class=\"headerlink\" href=\"#define-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
