selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_clients_grpc_internal.cpp.html#file-workspace-amdinfer-src-amdinfer-clients-grpc-internal-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File grpc_internal.cpp<a class=\"headerlink\" href=\"#file-grpc-internal-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_clients.html#dir-workspace-amdinfer-src-amdinfer-clients\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/clients</span></code>)</p><p>Implements the internal objects used for gRPC.</p>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-amdinfer-mapprototoparameters-const-google-protobuf-map-std-string-inference-inferparameter-parametermap\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::mapProtoToParameters(const google::protobuf::Map&lt;std::string, inference::InferParameter&gt;&amp;, ParameterMap *)<a class=\"headerlink\" href=\"#function-amdinfer-mapprototoparameters-const-google-protobuf-map-std-string-inference-inferparameter-parametermap\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
