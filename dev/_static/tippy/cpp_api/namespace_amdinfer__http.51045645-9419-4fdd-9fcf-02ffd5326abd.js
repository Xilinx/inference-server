selector_to_html = {"a[href=\"#classes\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Classes<a class=\"headerlink\" href=\"#classes\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"function_http__server_8cpp_1a024aac815aaac4c7a062bd1afa1d5efc.html#exhale-function-http-server-8cpp-1a024aac815aaac4c7a062bd1afa1d5efc\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::http::stop<a class=\"headerlink\" href=\"#function-amdinfer-http-stop\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"function_websocket__server_8cpp_1a59635d29a247f707bb9a44d0fb6aea2d.html#exhale-function-websocket-server-8cpp-1a59635d29a247f707bb9a44d0fb6aea2d\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::http::setCallback<a class=\"headerlink\" href=\"#function-amdinfer-http-setcallback\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#functions\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Functions<a class=\"headerlink\" href=\"#functions\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"function_http__server_8cpp_1a51d427f7d55612404bc5ed3f0441efb7.html#exhale-function-http-server-8cpp-1a51d427f7d55612404bc5ed3f0441efb7\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::http::start<a class=\"headerlink\" href=\"#function-amdinfer-http-start\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classamdinfer_1_1http_1_1WebsocketServer.html#exhale-class-classamdinfer-1-1http-1-1websocketserver\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class WebsocketServer<a class=\"headerlink\" href=\"#class-websocketserver\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Inheritance Relationships<a class=\"headerlink\" href=\"#inheritance-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Base Type<a class=\"headerlink\" href=\"#base-type\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#namespace-amdinfer-http\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Namespace amdinfer::http<a class=\"headerlink\" href=\"#namespace-amdinfer-http\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Classes<a class=\"headerlink\" href=\"#classes\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
