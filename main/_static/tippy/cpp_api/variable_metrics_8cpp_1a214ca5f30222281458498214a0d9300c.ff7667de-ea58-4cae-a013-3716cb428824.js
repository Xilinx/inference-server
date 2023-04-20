selector_to_html = {"a[href=\"#variable-amdinfer-kpercentile90\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Variable amdinfer::kPercentile90<a class=\"headerlink\" href=\"#variable-amdinfer-kpercentile90\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#variable-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer13kPercentile90E\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer13kPercentile90E\">\n<span id=\"_CPPv3N8amdinfer13kPercentile90E\"></span><span id=\"_CPPv2N8amdinfer13kPercentile90E\"></span><span id=\"amdinfer::kPercentile90__prometheus::detail::CKMSQuantiles::QuantileC\"></span><span class=\"target\" id=\"metrics_8cpp_1a214ca5f30222281458498214a0d9300c\"></span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">prometheus</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">detail</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">CKMSQuantiles</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">Quantile</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">kPercentile90</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">{</span></span><span class=\"m\"><span class=\"pre\">0.9</span></span><span class=\"p\"><span class=\"pre\">,</span></span><span class=\"w\"> </span><span class=\"m\"><span class=\"pre\">0.01</span></span><span class=\"p\"><span class=\"pre\">}</span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_observation_metrics.cpp.html#file-workspace-amdinfer-src-amdinfer-observation-metrics-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File metrics.cpp<a class=\"headerlink\" href=\"#file-metrics-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_observation.html#dir-workspace-amdinfer-src-amdinfer-observation\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/observation</span></code>)</p><p>Implements what metrics are defined and how they\u2019re are gathered.</p>"}
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
