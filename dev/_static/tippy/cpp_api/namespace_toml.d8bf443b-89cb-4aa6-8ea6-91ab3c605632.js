selector_to_html = {"a[href=\"#namespaces\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Namespaces<a class=\"headerlink\" href=\"#namespaces\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"namespace_toml__v3.html#namespace-toml-v3\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace toml::v3<a class=\"headerlink\" href=\"#namespace-toml-v3\" title=\"Permalink to this heading\">\u00b6</a></h1>", "a[href=\"#namespace-toml\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace toml<a class=\"headerlink\" href=\"#namespace-toml\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Namespaces<a class=\"headerlink\" href=\"#namespaces\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
