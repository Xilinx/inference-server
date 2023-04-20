selector_to_html = {"a[href=\"#function-amdinfer-operator-std-ostream-inferencerequestinput-const\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::operator&lt;&lt;(std::ostream&amp;, InferenceRequestInput const&amp;)<a class=\"headerlink\" href=\"#function-amdinfer-operator-std-ostream-inferencerequestinput-const\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"classamdinfer_1_1InferenceRequestInput.html#_CPPv4N8amdinfer21InferenceRequestInputE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer21InferenceRequestInputE\">\n<span id=\"_CPPv3N8amdinfer21InferenceRequestInputE\"></span><span id=\"_CPPv2N8amdinfer21InferenceRequestInputE\"></span><span id=\"amdinfer::InferenceRequestInput\"></span><span class=\"target\" id=\"classamdinfer_1_1InferenceRequestInput\"></span><span class=\"k\"><span class=\"pre\">class</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">InferenceRequestInput</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">:</span></span><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">public</span></span><span class=\"w\"> </span><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><a class=\"reference internal\" href=\"classamdinfer_1_1InferenceTensor.html#_CPPv4N8amdinfer15InferenceTensorE\" title=\"amdinfer::InferenceTensor\"><span class=\"n\"><span class=\"pre\">InferenceTensor</span></span></a><br/></dt><dd><p>Holds an inference request\u2019s input data. </p></dd>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinferlsERNSt7ostreamERK21InferenceRequestInput\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinferlsERNSt7ostreamERK21InferenceRequestInput\">\n<span id=\"_CPPv3N8amdinferlsERNSt7ostreamERK21InferenceRequestInput\"></span><span id=\"_CPPv2N8amdinferlsERNSt7ostreamERK21InferenceRequestInput\"></span><span id=\"amdinfer::lshift-operator__osR.InferenceRequestInputCR\"></span><span class=\"target\" id=\"inference__request_8cpp_1ac5a218218c06390bff837a4d000785c0\"></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">ostream</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"k\"><span class=\"pre\">operator</span></span><span class=\"o\"><span class=\"pre\">&lt;&lt;</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">ostream</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">os</span></span>, <a class=\"reference internal\" href=\"classamdinfer_1_1InferenceRequestInput.html#_CPPv4N8amdinfer21InferenceRequestInputE\" title=\"amdinfer::InferenceRequestInput\"><span class=\"n\"><span class=\"pre\">InferenceRequestInput</span></span></a><span class=\"w\"> </span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">my_class</span></span><span class=\"sig-paren\">)</span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_inference_request.cpp.html#file-workspace-amdinfer-src-amdinfer-core-inference-request-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File inference_request.cpp<a class=\"headerlink\" href=\"#file-inference-request-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>"}
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
