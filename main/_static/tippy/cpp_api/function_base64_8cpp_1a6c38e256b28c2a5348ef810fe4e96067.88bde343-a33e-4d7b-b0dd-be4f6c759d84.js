selector_to_html = {"a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer4util12base64EncodeEPKc6size_t\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer4util12base64EncodeEPKc6size_t\">\n<span id=\"_CPPv3N8amdinfer4util12base64EncodeEPKc6size_t\"></span><span id=\"_CPPv2N8amdinfer4util12base64EncodeEPKc6size_t\"></span><span id=\"amdinfer::util::base64Encode__cCP.s\"></span><span class=\"target\" id=\"base64_8cpp_1a6c38e256b28c2a5348ef810fe4e96067\"></span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">string</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">util</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">base64Encode</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">char</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">in</span></span>, <span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">in_len</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>Encodes a string with base64 and returns it. </p></dd>", "a[href=\"#function-amdinfer-util-base64encode-const-char-size-t\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::util::base64Encode(const char *, size_t)<a class=\"headerlink\" href=\"#function-amdinfer-util-base64encode-const-char-size-t\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_util_base64.cpp.html#file-workspace-amdinfer-src-amdinfer-util-base64-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File base64.cpp<a class=\"headerlink\" href=\"#file-base64-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_util.html#dir-workspace-amdinfer-src-amdinfer-util\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/util</span></code>)</p><p>Implements base64 encoding/decoding.</p>"}
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
