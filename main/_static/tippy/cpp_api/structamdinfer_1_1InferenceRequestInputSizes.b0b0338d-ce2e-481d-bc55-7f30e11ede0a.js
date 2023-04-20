selector_to_html = {"a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer26InferenceRequestInputSizes4dataE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer26InferenceRequestInputSizes4dataE\">\n<span id=\"_CPPv3N8amdinfer26InferenceRequestInputSizes4dataE\"></span><span id=\"_CPPv2N8amdinfer26InferenceRequestInputSizes4dataE\"></span><span id=\"amdinfer::InferenceRequestInputSizes::data__s\"></span><span class=\"target\" id=\"structamdinfer_1_1InferenceRequestInputSizes_1a0a11d4a4376004b76ed28bd476512341\"></span><span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">data</span></span></span><br/></dt><dd></dd>", "a[href=\"#struct-inferencerequestinputsizes\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Struct InferenceRequestInputSizes<a class=\"headerlink\" href=\"#struct-inferencerequestinputsizes\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_inference_request.cpp.html#file-workspace-amdinfer-src-amdinfer-core-inference-request-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File inference_request.cpp<a class=\"headerlink\" href=\"#file-inference-request-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>", "a[href=\"#_CPPv4N8amdinfer26InferenceRequestInputSizesE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer26InferenceRequestInputSizesE\">\n<span id=\"_CPPv3N8amdinfer26InferenceRequestInputSizesE\"></span><span id=\"_CPPv2N8amdinfer26InferenceRequestInputSizesE\"></span><span id=\"amdinfer::InferenceRequestInputSizes\"></span><span class=\"target\" id=\"structamdinfer_1_1InferenceRequestInputSizes\"></span><span class=\"k\"><span class=\"pre\">struct</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">InferenceRequestInputSizes</span></span></span><br/></dt><dd></dd>"}
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
