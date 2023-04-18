selector_to_html = {"a[href=\"classamdinfer_1_1HttpClient.html#exhale-class-classamdinfer-1-1httpclient\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class HttpClient<a class=\"headerlink\" href=\"#class-httpclient\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><h3>Nested Types<a class=\"headerlink\" href=\"#nested-types\" title=\"Permalink to this heading\">\u00b6</a></h3>", "a[href=\"#nested-relationships\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1HttpClient.html#exhale-class-classamdinfer-1-1httpclient\"><span class=\"std std-ref\">Class HttpClient</span></a>.</p>", "a[href=\"#class-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Class Documentation<a class=\"headerlink\" href=\"#class-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#class-httpclient-httpclientimpl\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Class HttpClient::HttpClientImpl<a class=\"headerlink\" href=\"#class-httpclient-httpclientimpl\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Nested Relationships<a class=\"headerlink\" href=\"#nested-relationships\" title=\"Permalink to this heading\">\u00b6</a></h2><p>This class is a nested type of <a class=\"reference internal\" href=\"classamdinfer_1_1HttpClient.html#exhale-class-classamdinfer-1-1httpclient\"><span class=\"std std-ref\">Class HttpClient</span></a>.</p>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_http.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-http-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File http.cpp<a class=\"headerlink\" href=\"#file-http-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the methods for interacting with the server with HTTP/REST.</p>", "a[href=\"typedef_declarations_8hpp_1a416c17e218dbbbba993f2f383a003ecf.html#_CPPv4N8amdinfer9StringMapE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer9StringMapE\">\n<span id=\"_CPPv3N8amdinfer9StringMapE\"></span><span id=\"_CPPv2N8amdinfer9StringMapE\"></span><span class=\"target\" id=\"declarations_8hpp_1a416c17e218dbbbba993f2f383a003ecf\"></span><span class=\"k\"><span class=\"pre\">using</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">StringMap</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">unordered_map</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"p\"><span class=\"pre\">,</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/></dt><dd></dd>"}
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
