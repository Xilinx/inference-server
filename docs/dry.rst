..
    Copyright 2023 Advanced Micro Devices, Inc.

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

..* This page has blocks of text that are inserted in multiple pages in an
..* effort to avoid duplicating text and making the documentation easier to
..* maintain. The format for text snippets is:
..*     +<label>
..*     <contents>
..*     -<label>
..* As an arbitrary convention, I'm using +/- prefixes to denote start and stop
..* labels. Note that due to how including sections of pages works in RST, the
..* labels must be entirely unique i.e. if you use "foobar" as a label, you
..* cannot use "foobar" at the start of any other label. Therefore, prefer
..* verbose labels to prevent unintentional matching.

+define_ensembles
a logical pipeline of workers to execute a graph of computations where the output tensors of one model are passed as input to others
-define_ensembles

+define_model_repository
a directory that exists on the host machine where the server :term:`container <Container (Docker)>`` is running and it holds the models you want to serve and their associated metadata in a standard structure
-define_model_repository
