selector_to_html = {"a[href=\"#classes\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Classes<a class=\"headerlink\" href=\"#classes\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classgoogle_1_1protobuf_1_1Map.html#exhale-class-classgoogle-1-1protobuf-1-1map\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Class Map<a class=\"headerlink\" href=\"#template-class-map\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#namespace-google-protobuf\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace google::protobuf<a class=\"headerlink\" href=\"#namespace-google-protobuf\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Classes<a class=\"headerlink\" href=\"#classes\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
