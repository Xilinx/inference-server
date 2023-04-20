selector_to_html = {"a[href=\"#_CPPv4N8amdinfer4util11readNthLineERKNSt6stringEi\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer4util11readNthLineERKNSt6stringEi\">\n<span id=\"_CPPv3N8amdinfer4util11readNthLineERKNSt6stringEi\"></span><span id=\"_CPPv2N8amdinfer4util11readNthLineERKNSt6stringEi\"></span><span id=\"amdinfer::util::readNthLine__ssCR.i\"></span><span class=\"target\" id=\"read__nth__line_8cpp_1ab24290f139bfd0ba4a626bdd7d5412d4\"></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">util</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">readNthLine</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">&amp;</span></span><span class=\"n sig-param\"><span class=\"pre\">filename</span></span>, <span class=\"kt\"><span class=\"pre\">int</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">n</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>This method will read the class file and returns class name. </p></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_read_nth_line.cpp.html#file-workspace-amdinfer-src-amdinfer-util-read-nth-line-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File read_nth_line.cpp<a class=\"headerlink\" href=\"#file-read-nth-line-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#function-amdinfer-util-readnthline\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::util::readNthLine<a class=\"headerlink\" href=\"#function-amdinfer-util-readnthline\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
