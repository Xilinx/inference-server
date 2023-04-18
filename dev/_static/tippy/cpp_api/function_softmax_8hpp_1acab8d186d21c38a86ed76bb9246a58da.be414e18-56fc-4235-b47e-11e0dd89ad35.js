selector_to_html = {"a[href=\"file__workspace_amdinfer_src_amdinfer_pre_post_softmax.hpp.html#file-workspace-amdinfer-src-amdinfer-pre-post-softmax-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File softmax.hpp<a class=\"headerlink\" href=\"#file-softmax-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_pre_post.html#dir-workspace-amdinfer-src-amdinfer-pre-post\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/pre_post</span></code>)</p>", "a[href=\"#function-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4I0EN8amdinfer8pre_post11calcSoftmaxEvPK1T6size_tPd\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4I0EN8amdinfer8pre_post11calcSoftmaxEvPK1T6size_tPd\">\n<span id=\"_CPPv3I0EN8amdinfer8pre_post11calcSoftmaxEPK1T6size_tPd\"></span><span id=\"_CPPv2I0EN8amdinfer8pre_post11calcSoftmaxEPK1T6size_tPd\"></span><span class=\"k\"><span class=\"pre\">template</span></span><span class=\"p\"><span class=\"pre\">&lt;</span></span><span class=\"k\"><span class=\"pre\">typename</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">T</span></span></span><span class=\"p\"><span class=\"pre\">&gt;</span></span><br/><span class=\"target\" id=\"softmax_8hpp_1acab8d186d21c38a86ed76bb9246a58da\"></span><span class=\"kt\"><span class=\"pre\">void</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">pre_post</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">calcSoftmax</span></span></span><span class=\"sig-paren\">(</span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><a class=\"reference internal\" href=\"#_CPPv4I0EN8amdinfer8pre_post11calcSoftmaxEvPK1T6size_tPd\" title=\"amdinfer::pre_post::calcSoftmax::T\"><span class=\"n\"><span class=\"pre\">T</span></span></a><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">data</span></span>, <span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"n sig-param\"><span class=\"pre\">size</span></span>, <span class=\"kt\"><span class=\"pre\">double</span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">*</span></span><span class=\"n sig-param\"><span class=\"pre\">result</span></span><span class=\"sig-paren\">)</span><br/></dt><dd><p>Calculate softmax of the data. </p></dd>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#template-function-amdinfer-pre-post-calcsoftmax\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Template Function amdinfer::pre_post::calcSoftmax<a class=\"headerlink\" href=\"#template-function-amdinfer-pre-post-calcsoftmax\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Function Documentation<a class=\"headerlink\" href=\"#function-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>"}
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
