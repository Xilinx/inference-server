selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_version.hpp.html#file-workspace-amdinfer-src-amdinfer-version-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File version.hpp<a class=\"headerlink\" href=\"#file-version-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer.html#dir-workspace-amdinfer-src-amdinfer\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer</span></code>)</p><p>Defines the version information.</p>", "a[href=\"#variable-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#variable-kamdinferversionminor\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Variable kAmdinferVersionMinor<a class=\"headerlink\" href=\"#variable-kamdinferversionminor\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv421kAmdinferVersionMinor\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv421kAmdinferVersionMinor\">\n<span id=\"_CPPv321kAmdinferVersionMinor\"></span><span id=\"_CPPv221kAmdinferVersionMinor\"></span><span id=\"kAmdinferVersionMinor__auto\"></span><span class=\"target\" id=\"version_8hpp_1a7d0f0ec1f70724c293b7d692a857dcf7\"></span><span class=\"k\"><span class=\"pre\">constexpr</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">auto</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">kAmdinferVersionMinor</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"m\"><span class=\"pre\">4</span></span><br/></dt><dd><p>minor version as an integer </p></dd>"}
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
