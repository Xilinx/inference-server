selector_to_html = {"a[href=\"#variable-amdinfer-pre-post-kdefaultimagesize\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Variable amdinfer::pre_post::kDefaultImageSize<a class=\"headerlink\" href=\"#variable-amdinfer-pre-post-kdefaultimagesize\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#variable-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Variable Documentation<a class=\"headerlink\" href=\"#variable-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"../cpp_user_api.html#_CPPv48amdinfer\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv48amdinfer\">\n<span id=\"_CPPv38amdinfer\"></span><span id=\"_CPPv28amdinfer\"></span><span id=\"amdinfer\"></span><span class=\"target\" id=\"namespaceamdinfer\"></span><span class=\"k\"><span class=\"pre\">namespace</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer8pre_post17kDefaultImageSizeE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer8pre_post17kDefaultImageSizeE\">\n<span id=\"_CPPv3N8amdinfer8pre_post17kDefaultImageSizeE\"></span><span id=\"_CPPv2N8amdinfer8pre_post17kDefaultImageSizeE\"></span><span id=\"amdinfer::pre_post::kDefaultImageSize__autoC\"></span><span class=\"target\" id=\"image__preprocess_8hpp_1a08c9e02a82e9fb21f8aa57bbbd33f77f\"></span><span class=\"k\"><span class=\"pre\">const</span></span><span class=\"w\"> </span><span class=\"kt\"><span class=\"pre\">auto</span></span><span class=\"w\"> </span><span class=\"sig-prename descclassname\"><a class=\"reference internal\" href=\"../cpp_user_api.html#_CPPv48amdinfer\" title=\"amdinfer\"><span class=\"n\"><span class=\"pre\">amdinfer</span></span></a><span class=\"p\"><span class=\"pre\">::</span></span><span class=\"n\"><span class=\"pre\">pre_post</span></span><span class=\"p\"><span class=\"pre\">::</span></span></span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">kDefaultImageSize</span></span></span><span class=\"w\"> </span><span class=\"p\"><span class=\"pre\">=</span></span><span class=\"w\"> </span><span class=\"m\"><span class=\"pre\">224</span></span><br/></dt><dd></dd>", "a[href=\"file__workspace_amdinfer_src_amdinfer_pre_post_image_preprocess.hpp.html#file-workspace-amdinfer-src-amdinfer-pre-post-image-preprocess-hpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File image_preprocess.hpp<a class=\"headerlink\" href=\"#file-image-preprocess-hpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_pre_post.html#dir-workspace-amdinfer-src-amdinfer-pre-post\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/pre_post</span></code>)</p>"}
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
