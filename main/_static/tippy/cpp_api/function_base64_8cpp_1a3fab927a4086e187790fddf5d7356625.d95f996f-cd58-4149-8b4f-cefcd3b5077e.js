selector_to_html = {"a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#function-amdinfer-util-base64decode-std-string\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::util::base64Decode(std::string)<a class=\"headerlink\" href=\"#function-amdinfer-util-base64decode-std-string\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_base64.cpp.html#file-workspace-amdinfer-src-amdinfer-util-base64-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File base64.cpp<a class=\"headerlink\" href=\"#file-base64-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p><p>Implements base64 encoding/decoding.</p>", "a[href=\"#_CPPv4N8amdinfer4util12base64DecodeENSt6stringE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer4util12base64DecodeENSt6stringE\">\n<span id=\"_CPPv3N8amdinfer4util12base64DecodeENSt6stringE\"></span><span id=\"_CPPv2N8amdinfer4util12base64DecodeENSt6stringE\"></span><span id=\"amdinfer::util::base64Decode__ss\"></span><span class=\"target\" id=\"base64_8cpp_1a3fab927a4086e187790fddf5d7356625\"></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">util</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">base64Decode</span></span></span><span class=\"sig-paren\">(</span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">in</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>Decodes a base64-encoded string and returns it. </p></dd>"}
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
