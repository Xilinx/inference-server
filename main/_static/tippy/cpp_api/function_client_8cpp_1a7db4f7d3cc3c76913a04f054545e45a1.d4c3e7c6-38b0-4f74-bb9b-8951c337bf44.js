selector_to_html = {"a[href=\"#function-amdinfer-inferasyncordered\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::inferAsyncOrdered<a class=\"headerlink\" href=\"#function-amdinfer-inferasyncordered\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer17inferAsyncOrderedEP6ClientRKNSt6stringERKNSt6vectorI16InferenceRequestEE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer17inferAsyncOrderedEP6ClientRKNSt6stringERKNSt6vectorI16InferenceRequestEE\">\n<span id=\"_CPPv3N8amdinfer17inferAsyncOrderedEP6ClientRKNSt6stringERKNSt6vectorI16InferenceRequestEE\"></span><span id=\"_CPPv2N8amdinfer17inferAsyncOrderedEP6ClientRKNSt6stringERKNSt6vectorI16InferenceRequestEE\"></span><span id=\"amdinfer::inferAsyncOrdered__ClientP.ssCR.std::vector:InferenceRequest:CR\"></span><span class=\"target\" id=\"client_8cpp_1a7db4f7d3cc3c76913a04f054545e45a1\"></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">vector</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><a class=\"reference internal\" href=\"classamdinfer_1_1InferenceResponse.html#_CPPv4N8amdinfer17InferenceResponseE\" title=\"amdinfer::InferenceResponse\"><span class=\"n\"><span class=\"pre\">InferenceResponse</span></span></a><span class=\"p\"><span class=\"pre\">&gt;</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">inferAsyncOrdered</span></span></span><span class=\"sig-paren\">(</span><a class=\"reference internal\" href=\"classamdinfer_1_1Client.html#_CPPv4N8amdinfer6ClientE\" title=\"amdinfer::Client\"><span class=\"n\"><span class=\"pre\">Client</span></span></a><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">client</span></span>, <span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">model</span></span>, <span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">vector</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><a class=\"reference internal\" href=\"classamdinfer_1_1InferenceRequest.html#_CPPv4N8amdinfer16InferenceRequestE\" title=\"amdinfer::InferenceRequest\"><span class=\"n\"><span class=\"pre\">InferenceRequest</span></span></a><span class=\"p\"><span class=\"pre\">&gt;</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">requests</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>Makes inference requests in parallel to the specified model. All requests are sent in parallel and the responses are gathered and returned in the same order. </p></dd>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"classamdinfer_1_1InferenceResponse.html#_CPPv4N8amdinfer17InferenceResponseE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer17InferenceResponseE\">\n<span id=\"_CPPv3N8amdinfer17InferenceResponseE\"></span><span id=\"_CPPv2N8amdinfer17InferenceResponseE\"></span><span id=\"amdinfer::InferenceResponse\"></span><span class=\"target\" id=\"classamdinfer_1_1InferenceResponse\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">InferenceResponse</span></span></span><br/></dt><dd><p>Creates an inference response object based on KServe\u2019s V2 spec that is used to respond back to clients. </p></dd>", "a[href=\"classamdinfer_1_1Client.html#_CPPv4N8amdinfer6ClientE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer6ClientE\">\n<span id=\"_CPPv3N8amdinfer6ClientE\"></span><span id=\"_CPPv2N8amdinfer6ClientE\"></span><span id=\"amdinfer::Client\"></span><span class=\"target\" id=\"classamdinfer_1_1Client\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">Client</span></span></span><br/></dt><dd><p>The base <a class=\"reference internal\" href=\"#classamdinfer_1_1Client\"><span class=\"std std-ref\">Client</span></a> class defines the set of methods that all client implementations must provide. These methods are based on the API defined by KServe, with some extensions. This is a pure virtual class. </p><p>Subclassed by <a class=\"reference internal\" href=\"../cpp_user_api.html#classamdinfer_1_1GrpcClient\"><span class=\"std std-ref\">amdinfer::GrpcClient</span></a>, <a class=\"reference internal\" href=\"../cpp_user_api.html#classamdinfer_1_1HttpClient\"><span class=\"std std-ref\">amdinfer::HttpClient</span></a>, <a class=\"reference internal\" href=\"../cpp_user_api.html#classamdinfer_1_1NativeClient\"><span class=\"std std-ref\">amdinfer::NativeClient</span></a>, <a class=\"reference internal\" href=\"../cpp_user_api.html#classamdinfer_1_1WebSocketClient\"><span class=\"std std-ref\">amdinfer::WebSocketClient</span></a></p></dd>", "a[href=\"classamdinfer_1_1InferenceRequest.html#_CPPv4N8amdinfer16InferenceRequestE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer16InferenceRequestE\">\n<span id=\"_CPPv3N8amdinfer16InferenceRequestE\"></span><span id=\"_CPPv2N8amdinfer16InferenceRequestE\"></span><span id=\"amdinfer::InferenceRequest\"></span><span class=\"target\" id=\"classamdinfer_1_1InferenceRequest\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">InferenceRequest</span></span></span><br/></dt><dd><p>Creates an inference request object based on KServe\u2019s V2 spec that is used to communicate between workers. </p></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_clients_client.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-client-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File client.cpp<a class=\"headerlink\" href=\"#file-client-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p>"}
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
