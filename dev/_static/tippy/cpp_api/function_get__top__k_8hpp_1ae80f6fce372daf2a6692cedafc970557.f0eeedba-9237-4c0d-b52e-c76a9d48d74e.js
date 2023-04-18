selector_to_html = {"a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer8pre_post7getTopKEPKd6size_ti\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer8pre_post7getTopKEPKd6size_ti\">\n<span id=\"_CPPv3N8amdinfer8pre_post7getTopKEPKd6size_ti\"></span><span id=\"_CPPv2N8amdinfer8pre_post7getTopKEPKd6size_ti\"></span><span id=\"amdinfer::pre_post::getTopK__doubleCP.s.i\"></span><span class=\"target\" id=\"get__top__k_8hpp_1ae80f6fce372daf2a6692cedafc970557\"></span><span class=\"k\"><span class=\"pre\">inline</span></span><span class=\"w\"> </span><span class=\"n\"><span class=\"pre\">std</span></span><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">vector</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"kt\"><span class=\"pre\">int</span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">pre_post</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">getTopK</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">double</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">d</span></span>, <span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">size</span></span>, <span class=\"kt\"><span class=\"pre\">int</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">k</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>After running softmax, get the labels associated with the top k values. </p></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_pre_post_get_top_k.hpp.html#file-workspace-amdinfer-src-amdinfer-pre-post-get-top-k-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File get_top_k.hpp<a class=\"headerlink\" href=\"#file-get-top-k-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_pre_post.html#dir-workspace-amdinfer-src-amdinfer-pre-post\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/pre_post</span></code>)</p>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#function-amdinfer-pre-post-gettopk\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Function amdinfer::pre_post::getTopK<a class=\"headerlink\" href=\"#function-amdinfer-pre-post-gettopk\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
