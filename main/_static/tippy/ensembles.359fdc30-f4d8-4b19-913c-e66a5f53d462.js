selector_to_html = {"a[href=\"glossary.html#term-Chain\"]": "<dt id=\"term-Chain\">Chain</dt><dd><p>a linear <a class=\"reference internal\" href=\"#term-Ensemble\"><span class=\"xref std std-term\">ensemble</span></a> where all the output tensors of one stage are inputs to the same next stage without having loops, broadcasts or concatenations</p></dd>"}
skip_classes = ["headerlink", "sd-stretched-link"]

window.onload = function () {
    for (const [select, tip_html] of Object.entries(selector_to_html)) {
        const links = document.querySelectorAll(` ${select}`);
        for (const link of links) {
            if (skip_classes.some(c => link.classList.contains(c))) {
                continue;
            }
            link.classList.add('has-tippy');
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
