selector_to_html = {"a[href=\"variable_c__plus__plus_8cpp_1a99f0d6272c799cd29afc336c68feb898.html#_CPPv4N8amdinfer13kInputTensorsE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer13kInputTensorsE\">\n<span id=\"_CPPv3N8amdinfer13kInputTensorsE\"></span><span id=\"_CPPv2N8amdinfer13kInputTensorsE\"></span><span id=\"amdinfer::kInputTensors__iC\"></span><span class=\"target\" id=\"c__plus__plus_8cpp_1a99f0d6272c799cd29afc336c68feb898\"></span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">int</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">kInputTensors</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"m\"><span class=\"pre\">2</span></span><br/></dt><dd></dd>", "a[href=\"#variable-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_workers_echo_multi.cpp.html#file-workspace-amdinfer-src-amdinfer-workers-echo-multi-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File echo_multi.cpp<a class=\"headerlink\" href=\"#file-echo-multi-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_workers.html#dir-workspace-amdinfer-src-amdinfer-workers\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/workers</span></code>)</p><p>Implements the EchoMulti worker.</p>", "a[href=\"#variable-amdinfer-kinputlengths\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Variable amdinfer::kInputLengths<a class=\"headerlink\" href=\"#variable-amdinfer-kinputlengths\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
