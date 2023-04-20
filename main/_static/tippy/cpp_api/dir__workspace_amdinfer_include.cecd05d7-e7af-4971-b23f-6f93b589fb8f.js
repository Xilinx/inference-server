selector_to_html = {"a[href=\"#directory-include\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Directory include<a class=\"headerlink\" href=\"#directory-include\" title=\"Permalink to this heading\">\u00b6</a></h1><p><em>Directory path:</em> <code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include</span></code></p>", "a[href=\"dir__workspace_amdinfer_include_amdinfer.html#dir-workspace-amdinfer-include-amdinfer\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Directory amdinfer<a class=\"headerlink\" href=\"#directory-amdinfer\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include.html#dir-workspace-amdinfer-include\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include</span></code>)</p><p><em>Directory path:</em> <code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer</span></code></p>", "a[href=\"#subdirectories\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Subdirectories<a class=\"headerlink\" href=\"#subdirectories\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
