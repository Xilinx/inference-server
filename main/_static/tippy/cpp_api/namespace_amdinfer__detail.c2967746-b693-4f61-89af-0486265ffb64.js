selector_to_html = {"a[href=\"function_data__types_8hpp_1a9f421600e40d463aae440ef86d990f77.html#exhale-function-data-types-8hpp-1a9f421600e40d463aae440ef86d990f77\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::detail::hash<a class=\"headerlink\" href=\"#function-amdinfer-detail-hash\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#functions\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#namespace-amdinfer-detail\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace amdinfer::detail<a class=\"headerlink\" href=\"#namespace-amdinfer-detail\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
