selector_to_html = {"a[href=\"#typedef-amdinfer-drogoncallback\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef amdinfer::DrogonCallback<a class=\"headerlink\" href=\"#typedef-amdinfer-drogoncallback\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#typedef-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Typedef Documentation<a class=\"headerlink\" href=\"#typedef-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_http_internal.hpp.html#file-workspace-amdinfer-src-amdinfer-clients-http-internal-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File http_internal.hpp<a class=\"headerlink\" href=\"#file-http-internal-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Defines the internal objects used for HTTP/REST.</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer14DrogonCallbackE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer14DrogonCallbackE\">\n<span id=\"_CPPv3N8amdinfer14DrogonCallbackE\"></span><span id=\"_CPPv2N8amdinfer14DrogonCallbackE\"></span><span class=\"target\" id=\"http__server_8cpp_1a7137f2088c425db6fdd74b1be1fe0663\"></span><span class=\"k\"><span class=\"pre\">using</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">DrogonCallback</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">function</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"p\"><span class=\"pre\">(</span></span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">drogon</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">HttpResponsePtr</span></span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"p\"><span class=\"pre\">)</span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/></dt><dd></dd>"}
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
