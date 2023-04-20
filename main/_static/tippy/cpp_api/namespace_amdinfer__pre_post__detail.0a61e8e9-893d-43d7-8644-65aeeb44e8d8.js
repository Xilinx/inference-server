selector_to_html = {"a[href=\"#functions\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#namespace-amdinfer-pre-post-detail\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace amdinfer::pre_post::detail<a class=\"headerlink\" href=\"#namespace-amdinfer-pre-post-detail\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"function_image__preprocess_8hpp_1a85688ea858ae957a61a92a169608ff0c.html#exhale-function-image-preprocess-8hpp-1a85688ea858ae957a61a92a169608ff0c\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Function amdinfer::pre_post::detail::normalize<a class=\"headerlink\" href=\"#template-function-amdinfer-pre-post-detail-normalize\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"function_image__preprocess_8hpp_1afc409bcd827fd1219cc6844e0e07ed62.html#exhale-function-image-preprocess-8hpp-1afc409bcd827fd1219cc6844e0e07ed62\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Function amdinfer::pre_post::detail::nestedLoop<a class=\"headerlink\" href=\"#template-function-amdinfer-pre-post-detail-nestedloop\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
