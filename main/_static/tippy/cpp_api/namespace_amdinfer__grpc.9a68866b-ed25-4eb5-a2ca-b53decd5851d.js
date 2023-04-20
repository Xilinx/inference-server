selector_to_html = {"a[href=\"function_grpc__server_8cpp_1acd5880aba6baa79251eba9aa507f2dd4.html#exhale-function-grpc-server-8cpp-1acd5880aba6baa79251eba9aa507f2dd4\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::grpc::start<a class=\"headerlink\" href=\"#function-amdinfer-grpc-start\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#functions\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#namespace-amdinfer-grpc\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace amdinfer::grpc<a class=\"headerlink\" href=\"#namespace-amdinfer-grpc\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"function_grpc__server_8cpp_1a15ea0fa3e0ee2783642361b4e46de4b1.html#exhale-function-grpc-server-8cpp-1a15ea0fa3e0ee2783642361b4e46de4b1\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::grpc::stop<a class=\"headerlink\" href=\"#function-amdinfer-grpc-stop\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
