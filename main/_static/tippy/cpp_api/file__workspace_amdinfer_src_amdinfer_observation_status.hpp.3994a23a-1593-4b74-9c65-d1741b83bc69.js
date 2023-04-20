selector_to_html = {"a[href=\"program_listing_file__workspace_amdinfer_src_amdinfer_observation_status.hpp.html\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Program Listing for File status.hpp<a class=\"headerlink\" href=\"#program-listing-for-file-status-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"file__workspace_amdinfer_src_amdinfer_observation_status.hpp.html#file-workspace-amdinfer-src-amdinfer-observation-status-hpp\"><span class=\"std std-ref\">Return to documentation for file</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation/status.hpp</span></code>)</p>", "a[href=\"#namespaces\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Namespaces<a class=\"headerlink\" href=\"#namespaces\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"enum_status_8hpp_1a913436294d314192e3d1ef856c142de3.html#exhale-enum-status-8hpp-1a913436294d314192e3d1ef856c142de3\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Enum Status<a class=\"headerlink\" href=\"#enum-status\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Enum Documentation<a class=\"headerlink\" href=\"#enum-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#file-status-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File status.hpp<a class=\"headerlink\" href=\"#file-status-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_observation.html#dir-workspace-amdinfer-src-amdinfer-observation\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation</span></code>)</p><p>Defines the status codes.</p>", "a[href=\"#definition-workspace-amdinfer-src-amdinfer-observation-status-hpp\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Definition (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation/status.hpp</span></code>)<a class=\"headerlink\" href=\"#definition-workspace-amdinfer-src-amdinfer-observation-status-hpp\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#enums\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Enums<a class=\"headerlink\" href=\"#enums\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"namespace_amdinfer.html#namespace-amdinfer\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace amdinfer<a class=\"headerlink\" href=\"#namespace-amdinfer\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Namespaces<a class=\"headerlink\" href=\"#namespaces\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"dir__workspace_amdinfer_src_amdinfer_observation.html#dir-workspace-amdinfer-src-amdinfer-observation\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Directory observation<a class=\"headerlink\" href=\"#directory-observation\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer.html#dir-workspace-amdinfer-src-amdinfer\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer</span></code>)</p><p><em>Directory path:</em> <code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation</span></code></p>"}
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
