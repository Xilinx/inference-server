selector_to_html = {"a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer28InferenceResponseOutputSizes4dataE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer28InferenceResponseOutputSizes4dataE\">\n<span id=\"_CPPv3N8amdinfer28InferenceResponseOutputSizes4dataE\"></span><span id=\"_CPPv2N8amdinfer28InferenceResponseOutputSizes4dataE\"></span><span id=\"amdinfer::InferenceResponseOutputSizes::data__s\"></span><span class=\"target\" id=\"structamdinfer_1_1InferenceResponseOutputSizes_1a5709fb65a08b09409bd731de3712f1bf\"></span><span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">data</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_inference_response.cpp.html#file-workspace-amdinfer-src-amdinfer-core-inference-response-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File inference_response.cpp<a class=\"headerlink\" href=\"#file-inference-response-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>", "a[href=\"#struct-inferenceresponseoutputsizes\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Struct InferenceResponseOutputSizes<a class=\"headerlink\" href=\"#struct-inferenceresponseoutputsizes\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer28InferenceResponseOutputSizesE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer28InferenceResponseOutputSizesE\">\n<span id=\"_CPPv3N8amdinfer28InferenceResponseOutputSizesE\"></span><span id=\"_CPPv2N8amdinfer28InferenceResponseOutputSizesE\"></span><span id=\"amdinfer::InferenceResponseOutputSizes\"></span><span class=\"target\" id=\"structamdinfer_1_1InferenceResponseOutputSizes\"></span><span class=\"k\"><span class=\"pre\">struct</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">InferenceResponseOutputSizes</span></span></span><br/></dt><dd></dd>"}
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
