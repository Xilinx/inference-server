selector_to_html = {"a[href=\"#classes\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Classes<a class=\"headerlink\" href=\"#classes\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"structstd_1_1less_3_01amdinfer_1_1ParameterMap_01_4.html#exhale-struct-structstd-1-1less-3-01amdinfer-1-1parametermap-01-4\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Struct less&lt; amdinfer::ParameterMap &gt;<a class=\"headerlink\" href=\"#template-struct-less-amdinfer-parametermap\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#namespace-std\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace std<a class=\"headerlink\" href=\"#namespace-std\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Classes<a class=\"headerlink\" href=\"#classes\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
