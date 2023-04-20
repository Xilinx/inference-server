selector_to_html = {"a[href=\"classamdinfer_1_1Manager.html#exhale-class-classamdinfer-1-1manager\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class Manager<a class=\"headerlink\" href=\"#class-manager\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Nested Types<a class=\"headerlink\" href=\"#nested-types\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#nested-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1Manager.html#exhale-class-classamdinfer-1-1manager\"><span class=\"std std-ref\">Class Manager</span></a>.</p>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_manager.hpp.html#file-workspace-amdinfer-src-amdinfer-core-manager-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File manager.hpp<a class=\"headerlink\" href=\"#file-manager-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p><p>Defines how the shared mutable state is managed.</p>", "a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#class-manager-endpoints\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class Manager::Endpoints<a class=\"headerlink\" href=\"#class-manager-endpoints\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1Manager.html#exhale-class-classamdinfer-1-1manager\"><span class=\"std std-ref\">Class Manager</span></a>.</p>"}
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
