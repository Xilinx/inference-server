selector_to_html = {"a[href=\"glossary.html#term-Ensemble\"]": "<dt id=\"term-Ensemble\">Ensemble</dt><dd><p>a logical pipeline of workers to execute a graph of computations where the output tensors of one model are passed as input to others</p></dd>", "a[href=\"glossary.html#term-Chain\"]": "<dt id=\"term-Chain\">Chain</dt><dd><p>a linear <a class=\"reference internal\" href=\"#term-Ensemble\"><span class=\"xref std std-term\">ensemble</span></a> where all the output tensors of one stage are inputs to the same next stage without having loops, broadcasts or concatenations</p></dd>", "a[href=\"glossary.html#term-Container-Docker\"]": "<dt id=\"term-Container-Docker\">Container (Docker)</dt><dd><p>a standard unit of software that packages up code and all its dependencies so the application runs quickly and reliably from one computing environment to another <a class=\"footnote-reference brackets\" href=\"#id3\" id=\"id1\">1</a></p><p>see also: <a class=\"reference internal\" href=\"#term-Image-Docker\"><span class=\"xref std std-term\">image</span></a></p></dd>"}
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
