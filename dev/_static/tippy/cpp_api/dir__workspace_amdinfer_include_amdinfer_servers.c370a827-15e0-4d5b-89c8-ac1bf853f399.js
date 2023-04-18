selector_to_html = {"a[href=\"#files\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Files<a class=\"headerlink\" href=\"#files\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"dir__workspace_amdinfer_include_amdinfer.html#dir-workspace-amdinfer-include-amdinfer\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Directory amdinfer<a class=\"headerlink\" href=\"#directory-amdinfer\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include.html#dir-workspace-amdinfer-include\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include</span></code>)</p><p><em>Directory path:</em> <code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer</span></code></p>", "a[href=\"file__workspace_amdinfer_include_amdinfer_servers_server.hpp.html#file-workspace-amdinfer-include-amdinfer-servers-server-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File server.hpp<a class=\"headerlink\" href=\"#file-server-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer_servers.html#dir-workspace-amdinfer-include-amdinfer-servers\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer/servers</span></code>)</p>", "a[href=\"#directory-servers\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Directory servers<a class=\"headerlink\" href=\"#directory-servers\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_include_amdinfer.html#dir-workspace-amdinfer-include-amdinfer\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer</span></code>)</p><p><em>Directory path:</em> <code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/include/amdinfer/servers</span></code></p>"}
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
