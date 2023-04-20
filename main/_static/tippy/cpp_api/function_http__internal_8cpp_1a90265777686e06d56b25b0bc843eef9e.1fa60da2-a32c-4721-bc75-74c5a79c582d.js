selector_to_html = {"a[href=\"#function-amdinfer-errorhttpresponse\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::errorHttpResponse<a class=\"headerlink\" href=\"#function-amdinfer-errorhttpresponse\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_http_internal.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-http-internal-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File http_internal.cpp<a class=\"headerlink\" href=\"#file-http-internal-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the internal objects used for HTTP/REST.</p>", "a[href=\"#_CPPv4N8amdinfer17errorHttpResponseERKNSt6stringEi\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer17errorHttpResponseERKNSt6stringEi\">\n<span id=\"_CPPv3N8amdinfer17errorHttpResponseERKNSt6stringEi\"></span><span id=\"_CPPv2N8amdinfer17errorHttpResponseERKNSt6stringEi\"></span><span id=\"amdinfer::errorHttpResponse__ssCR.i\"></span><span class=\"target\" id=\"http__server_8cpp_1a90265777686e06d56b25b0bc843eef9e\"></span><span class=\"n\"><span class=\"pre\">drogon</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">HttpResponsePtr</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">errorHttpResponse</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">error</span></span>, <span class=\"kt\"><span class=\"pre\">int</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">status_code</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>"}
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
