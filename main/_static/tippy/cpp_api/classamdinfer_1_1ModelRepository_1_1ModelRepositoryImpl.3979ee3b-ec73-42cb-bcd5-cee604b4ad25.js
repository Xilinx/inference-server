selector_to_html = {"a[href=\"#nested-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1ModelRepository.html#exhale-class-classamdinfer-1-1modelrepository\"><span class=\"std std-ref\">Class ModelRepository</span></a>.</p>", "a[href=\"#class-modelrepository-modelrepositoryimpl\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class ModelRepository::ModelRepositoryImpl<a class=\"headerlink\" href=\"#class-modelrepository-modelrepositoryimpl\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1ModelRepository.html#exhale-class-classamdinfer-1-1modelrepository\"><span class=\"std std-ref\">Class ModelRepository</span></a>.</p>", "a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classamdinfer_1_1ModelRepository.html#exhale-class-classamdinfer-1-1modelrepository\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class ModelRepository<a class=\"headerlink\" href=\"#class-modelrepository\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_model_repository.hpp.html#file-workspace-amdinfer-src-amdinfer-core-model-repository-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File model_repository.hpp<a class=\"headerlink\" href=\"#file-model-repository-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>"}
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
