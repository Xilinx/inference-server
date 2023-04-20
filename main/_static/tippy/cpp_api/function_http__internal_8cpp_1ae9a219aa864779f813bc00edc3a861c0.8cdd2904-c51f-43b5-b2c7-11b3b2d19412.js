selector_to_html = {"a[href=\"classamdinfer_1_1ParameterMap.html#_CPPv4N8amdinfer12ParameterMapE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer12ParameterMapE\">\n<span id=\"_CPPv3N8amdinfer12ParameterMapE\"></span><span id=\"_CPPv2N8amdinfer12ParameterMapE\"></span><span id=\"amdinfer::ParameterMap\"></span><span class=\"target\" id=\"classamdinfer_1_1ParameterMap\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">ParameterMap</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">:</span></span><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">public</span></span><span class=\"w\"> </span><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><a class=\"reference internal\" href=\"classamdinfer_1_1Serializable.html#_CPPv4N8amdinfer12SerializableE\" title=\"amdinfer::Serializable\"><span class=\"n\"><span class=\"pre\">Serializable</span></span></a><br/></dt><dd><p>Holds any parameters from JSON (defined by KServe spec as one of bool, number or string). We further restrict numbers to be doubles or int32. </p></dd>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_http_internal.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-http-internal-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File http_internal.cpp<a class=\"headerlink\" href=\"#file-http-internal-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the internal objects used for HTTP/REST.</p>", "a[href=\"#function-amdinfer-mapjsontoparameters\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::mapJsonToParameters<a class=\"headerlink\" href=\"#function-amdinfer-mapjsontoparameters\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
