selector_to_html = {"a[href=\"#struct-nativeclient-nativeclientimpl\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Struct NativeClient::NativeClientImpl<a class=\"headerlink\" href=\"#struct-nativeclient-nativeclientimpl\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This struct is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1NativeClient.html#exhale-class-classamdinfer-1-1nativeclient\"><span class=\"std std-ref\">Class NativeClient</span></a>.</p>", "a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classamdinfer_1_1SharedState.html#_CPPv4N8amdinfer11SharedStateE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11SharedStateE\">\n<span id=\"_CPPv3N8amdinfer11SharedStateE\"></span><span id=\"_CPPv2N8amdinfer11SharedStateE\"></span><span id=\"amdinfer::SharedState\"></span><span class=\"target\" id=\"classamdinfer_1_1SharedState\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">SharedState</span></span></span><br/></dt><dd></dd>", "a[href=\"#nested-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This struct is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1NativeClient.html#exhale-class-classamdinfer-1-1nativeclient\"><span class=\"std std-ref\">Class NativeClient</span></a>.</p>", "a[href=\"classamdinfer_1_1NativeClient.html#exhale-class-classamdinfer-1-1nativeclient\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class NativeClient<a class=\"headerlink\" href=\"#class-nativeclient\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Nested Types<a class=\"headerlink\" href=\"#nested-types\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_native.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-native-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File native.cpp<a class=\"headerlink\" href=\"#file-native-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the methods for interacting with the server in the native C++ API.</p>"}
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
