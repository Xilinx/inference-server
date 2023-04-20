selector_to_html = {"a[href=\"#struct-tensorsizes\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">Struct TensorSizes<a class=\"headerlink\" href=\"#struct-tensorsizes\" title=\"Permalink to this heading\">\u00b6</a></h1><h2>Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"file__workspace_amdinfer_src_amdinfer_core_tensor.cpp.html#file-workspace-amdinfer-src-amdinfer-core-tensor-cpp\"]": "<h1 class=\"tippy-header\" style=\"margin-top: 0;\">File tensor.cpp<a class=\"headerlink\" href=\"#file-tensor-cpp\" title=\"Permalink to this heading\">\u00b6</a></h1><p>\u21b0 <a class=\"reference internal\" href=\"dir__workspace_amdinfer_src_amdinfer_core.html#dir-workspace-amdinfer-src-amdinfer-core\"><span class=\"std std-ref\">Parent directory</span></a> (<code class=\"docutils literal notranslate\"><span class=\"pre\">/workspace/amdinfer/src/amdinfer/core</span></code>)</p>", "a[href=\"#struct-documentation\"]": "<h2 class=\"tippy-header\" style=\"margin-top: 0;\">Struct Documentation<a class=\"headerlink\" href=\"#struct-documentation\" title=\"Permalink to this heading\">\u00b6</a></h2>", "a[href=\"#_CPPv4N8amdinfer11TensorSizes5shapeE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11TensorSizes5shapeE\">\n<span id=\"_CPPv3N8amdinfer11TensorSizes5shapeE\"></span><span id=\"_CPPv2N8amdinfer11TensorSizes5shapeE\"></span><span id=\"amdinfer::TensorSizes::shape__s\"></span><span class=\"target\" id=\"structamdinfer_1_1TensorSizes_1a644be2a5cd3c46aabb05fcaca2c521fa\"></span><span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">shape</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer11TensorSizes9data_typeE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11TensorSizes9data_typeE\">\n<span id=\"_CPPv3N8amdinfer11TensorSizes9data_typeE\"></span><span id=\"_CPPv2N8amdinfer11TensorSizes9data_typeE\"></span><span id=\"amdinfer::TensorSizes::data_type__s\"></span><span class=\"target\" id=\"structamdinfer_1_1TensorSizes_1a91c55c9825088a932d56671a459aa848\"></span><span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">data_type</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer11TensorSizesE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11TensorSizesE\">\n<span id=\"_CPPv3N8amdinfer11TensorSizesE\"></span><span id=\"_CPPv2N8amdinfer11TensorSizesE\"></span><span id=\"amdinfer::TensorSizes\"></span><span class=\"target\" id=\"structamdinfer_1_1TensorSizes\"></span><span class=\"k\"><span class=\"pre\">struct</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">TensorSizes</span></span></span><br/></dt><dd></dd>", "a[href=\"#_CPPv4N8amdinfer11TensorSizes4nameE\"]": "<dt class=\"sig sig-object cpp\" id=\"_CPPv4N8amdinfer11TensorSizes4nameE\">\n<span id=\"_CPPv3N8amdinfer11TensorSizes4nameE\"></span><span id=\"_CPPv2N8amdinfer11TensorSizes4nameE\"></span><span id=\"amdinfer::TensorSizes::name__s\"></span><span class=\"target\" id=\"structamdinfer_1_1TensorSizes_1a19d2a83702244a8a8af841aaed681cc4\"></span><span class=\"n\"><span class=\"pre\">size_t</span></span><span class=\"w\"> </span><span class=\"sig-name descname\"><span class=\"n\"><span class=\"pre\">name</span></span></span><br/></dt><dd></dd>"}
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
