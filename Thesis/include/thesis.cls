\ProvidesClass{include/thesis}
\LoadClass[a4paper, 11pt, twoside]{book}

% \usepackage[utf8]{inputenc}
\usepackage[T1]{fontenc}
\usepackage[ngerman, english]{babel}
\usepackage[margin=1.1in]{geometry}
\usepackage{amsfonts, amstext, amssymb}
\usepackage{amsmath}
\usepackage{mathtools}
\usepackage{graphicx}
\usepackage[export]{adjustbox}
\usepackage{ae, aecompl}
\usepackage{listings}
\usepackage[binary-units]{siunitx}
\usepackage{rotating}
\usepackage{subfig}
\usepackage{mparhack}
\usepackage{lipsum}
\usepackage{authoraftertitle}
\usepackage[printonlyused, withpage]{acronym}
\newcommand{\word}[2]{\acro{#1}{\textit{#2}}}


\usepackage[dvipsnames, table]{xcolor}
\usepackage{changes}
\usepackage{blindtext}
\usepackage[b]{esvect}    % For \vv
\usepackage{tikz}         % For arrow and dots in \xvec
\usetikzlibrary{calc}
\usepackage{xparse}
\usepackage{listings}
\usepackage{upgreek}
\usepackage{hyperref}

\lstdefinelanguage{numpy}{
  keywords = {
    and,del,from,not,while,
    as,elif,global,or,with,
    assert,else,if,pass,yield,
    break,except,import,print,
    class,exec,in, raise,continue,
    finally,is, return,def,for,
    lambda,try
    },
    morekeywords = {numpy,fft,mean}
    }
    
\lstdefinelanguage{XML}
{
  basicstyle=\ttfamily\color{darkblue}\bfseries\footnotesize,
  morestring=[b]",
  morestring=[s]{>}{<},
  morecomment=[s]{<?}{?>},
  stringstyle=\color{black},
  identifierstyle=\color{darkblue},
  keywordstyle=\color{cyan},
  morekeywords={xmlns,version,type},
  breaklines=true
  }
  
\lstset{
  basicstyle=\ttfamily,
  columns=fullflexible,
  showstringspaces=false,
  commentstyle=\color{gray}\upshape
  }

\usepackage[type={CC},modifier={by-nc-sa},version={3.0}]{doclicense}
    
\usepackage{color}
\definecolor{darkred}{rgb}{0.5,0,0}
\definecolor{firebrick}{rgb}{0.75,0.125,0.125}
\definecolor{darkgreen}{rgb}{0,0.5,0}
\definecolor{lightgray}{gray}{0.9}
\definecolor{gray}{rgb}{0.4,0.4,0.4}
\definecolor{darkblue}{rgb}{0.0,0.0,0.6}
\definecolor{cyan}{rgb}{0.0,0.6,0.6}
\usepackage{etoolbox}
\AtBeginEnvironment{acronym}{%
\renewcommand*\descriptionlabel[1]{\hspace\labelsep
\normalfont\bfseries\color{grey} #1}}

% adjust colour of acronyms
\makeatletter
\AtBeginDocument{%
  \renewcommand*{\AC@hyperlink}[2]{%
  \begingroup
  \hypersetup{hidelinks}%
      \hyperlink{#1}{#2}%
    \endgroup
  }%
}
\makeatother

\usepackage[rightcaption]{sidecap}
\sidecaptionvpos{figure}{t}
\usepackage{booktabs}
\usepackage{eurosym}
\usepackage{title/KITtitle}
\usepackage{todonotes}
\usepackage{python}
\usepackage{xspace}
\usepackage{parskip}

\usepackage[activate={true,nocompatibility},final,tracking=true,kerning=true,spacing=true,factor=1100,stretch=10,shrink=10]{microtype}
% activate={true,nocompatibility} - activate protrusion and expansion
% final - enable microtype; use "draft" to disable
% tracking=true, kerning=true, spacing=true - activate these techniques
% factor=1100 - add 10% to the protrusion amount (default is 1000)
% stretch=10, shrink=10 - reduce stretchability/shrinkability (default is 20/20)

\SetTracking{encoding={*}, shape=sc}{40}
\usepackage[Conny]{fncychap}
\ChTitleAsIs
\usepackage[labelfont=bf,font=small,margin=10pt,format=plain,tableposition=top]{caption}
\displaywidowpenalty = 10000      % Disable single lines at the end of a paragraph (Hurenkinder)
\clubpenalty = 10000              % Disable single lines at the start of a paragraph (Schusterjungen)
\widowpenalty = 10000
\usepackage{mathpazo}
\extrafloats{100}
\usepackage[style=numeric-comp, sorting=ynt, defernumbers=true]{biblatex}
\addbibresource{bib/thesis.bib}

% % Describe separation hints here:
% \hyphenation{
%   % Pro-to-koll-in-stan-zen
%   % Ma-na-ge-ment  Netz-werk-ele-men-ten
%   % Netz-werk Netz-werk-re-ser-vie-rung
%   % Netz-werk-adap-ter Fein-ju-stier-ung
%   % Da-ten-strom-spe-zi-fi-ka-tion Pa-ket-rumpf
%   % Kon-troll-in-stanz
%   % Me
%   }

  
\makeatletter
\DeclareRobustCommand*{\bfseries}{%
\not@math@alphabet\bfseries\mathbf
\fontseries\bfdefault\selectfont
\boldmath}
\makeatother
  
\input{include/utilities}

\graphicspath{{figures/}}
\setcounter{topnumber}{4}
\renewcommand{\topfraction}{1.}
\setcounter{bottomnumber}{0}
\renewcommand{\bottomfraction}{0}
\setcounter{totalnumber}{4}
\renewcommand{\textfraction}{0.}
\renewcommand{\floatpagefraction}{1.}

\sisetup{
  per-mode = symbol,
  detect-all,
  tight-spacing = true,
  separate-uncertainty = true
}
  
\let\oldbaselinestretch=\baselinestretch%
\renewcommand{\baselinestretch}{1.15}%
\large\normalsize%


% toc & title formatting
\usepackage[explicit]{titlesec}
\usepackage{titletoc}

\setcounter{secnumdepth}{3}
\setcounter{tocdepth}{3}

\titlecontents{part}                % what is changed
  [0.1cm]                           % left margin
  {\vspace{1cm}\hrule height 1.5pt} % code executed before toc-item
  {}                                % content if label is present
  {}                                % content if there is no label present
  {}                                % filler page format (like dots, etc.)
[\hrule height 1.5pt]               % code executed after toc-item


% custom chapters for frontmatter
\titleclass{\prepostchapter}{straight}[\part]
\titleformat{\prepostchapter}       % what is changed
  [display]                         % shape factor within text
  {\centering\normalfont\huge}      % format of title and label
  {}                                % label of title (chapter?)
  {0pt}                             % separation between label and title body
  {#1}                              % code executed after label, before title. #1 is the title
  [\hrule\vspace{-0.5cm}]           % code executed after title
\titlespacing*{\prepostchapter} {0pt}{50pt}{40pt}
\titlecontents{prepostchapter}
  [0.2cm]
  {\bfseries}
  {
    \IfInteger{\thecontentslabel}
    {\includegraphics[height=0.3cm]{../include/\thecontentslabel.png}\hspace{0.5em}}
    \thecontentslabel\hspace{0.5em}
  }
  {}
  {\titlerule*[1pc]{.}\contentspage}
\newcounter{prepostchapter}

% custom sections, subsections, and subsubsections for minitocs
\titlecontents{psection}[0.2cm]{\vspace{-0.3cm}\bfseries}{\thecontentslabel\hspace{0.5em}}{}{\normalfont\titlerule*[1pc]{.}\contentspage}
\titlecontents{psubsection}[5.1em]{\vspace{-0.3cm}}{\contentslabel{2.7em}}{}{\titlerule*[1pc]{.}\contentspage}
\titlecontents{psubsubsection}[5.7em]{\vspace{-0.3cm}}{\contentslabel{4.1em}}{}{\titlerule*[1pc]{.}\contentspage}


% chapter with automatic minitoc creation
\usepackage[pdftex, outline]{contour}
  \contourlength{0.8mm}
  \usepackage[nolinks]{qrcode}
  \newlength\qrlength
  \setlength\qrlength{2.5cm}
  \qrset{height=\qrlength}
  \usepackage{fancyhdr}
  \renewcommand{\chaptermark}[1]{\markboth{#1}{}}
  \newlength\titlesep
  \setlength\titlesep{0.4cm}
  \ExplSyntaxOn
  \NewDocumentCommand{\Contour}{m}
  {
    \seq_set_split:Nnn \l_tmpa_seq { ~ } { #1 }
    \seq_map_inline:Nn \l_tmpa_seq { \contour{white}{##1} ~ } \unskip
  }
  \ExplSyntaxOff

% chapter
% \titleformat{\chapter}
%     [display]
%     {}
%     {}
%     {0pt}
%     {%
%       \thispagestyle{empty}
%       \startcontents
%       \begin{tikzpicture}[overlay]
%         \node[anchor=north west] at (0, 0.847cm) 
%           {\textcolor{cyan}{\qrcode{https://github.com/Quizznor/phd-thesis/blob/main/Thesis/header/\thechapter.txt}}};
%         \draw[line width=0.5mm](\qrlength + \titlesep, 0.847cm-3.9pt)--(\qrlength + \titlesep, -\qrlength - 4pt + 0.847cm);
%         \node[anchor=south west] at (\qrlength + \titlesep + 1.7pt, -\qrlength - 9.8pt + 0.847cm)%
%           {\includegraphics[height=\qrlength+3.7pt, width=\textwidth-\qrlength-1.3\titlesep]{header/\thechapter.png}};
%         \end{tikzpicture}
%         \hfill
%         \begin{minipage}[t][1.3cm][b]{\textwidth-\qrlength-2.6\titlesep}
%             \centering\fontsize{25pt}{0pt}\selectfont\Contour{\ifnum\thechapter=0\fontsize{50}{0}\selectfont\else\fi#1}
%         \end{minipage}
%     }
%     [%
%     \ifnum\thechapter=0
%       \vspace{0.6cm}
%     \else
%       \vspace*{1cm}%
%       \normalsize\normalfont
%       \hfill\textbf{Contents}\hrule\vspace{0.1cm}
%       \printcontents{p}{2}{\setcounter{tocdepth}{4}}
%       \hrule\vspace{-0.6cm}
%     \fi
%     ]


\titleformat{\chapter}
  [display]
  {}
  {}
  {0pt}
  {%
    \thispagestyle{empty}
    \startcontents
    \ifnum\thechapter=0
      \centering\normalfont\huge #1
      \hrule\vspace{-1cm}
    \else
      \raisebox{-1.5pt}{\sc{\chaptername}~\thechapter}~\titlerule\newline
      \begin{flushright}
        \begin{minipage}[t]{0.85\textwidth}
          \begin{flushright}
            \bfseries\Huge \sc{#1}
          \end{flushright}
        \end{minipage}
      \end{flushright}
      \vspace{-0.7cm}
    \fi
  }
  [%
    \ifnum\thechapter=0
      \vspace{0.1cm}
    \else
      \raisebox{-1.5pt}{\sc{Contents}}~\titlerule\flushright
      \normalsize\normalfont\vspace{5pt}
      \printcontents{p}{2}{\setcounter{tocdepth}{4}}
      \vspace{5pt}\titlerule
    \fi
  ]
  \titlecontents{chapter}
    [0.2cm]
    {\bfseries\vspace{0.85cm}\hrule\vspace{-0.55cm}}
    {\hspace{0.4em}\begin{Large}\thecontentslabel\end{Large}\hspace{0.85em}\large}
    {}
    {\hfill\normalsize\contentspage}
    []
\titlespacing*{\chapter} {0pt}{-2cm}{0.5cm}


% sections
\titlecontents{section}
    [0.55cm]
    {}
    {\bfseries\thecontentslabel\hspace{0.5em}}
    {}
    {\normalfont\titlerule*[0.7pc]{.}\contentspage}
[]


% subsection
\titlecontents{subsection}
    [0.85cm]
    {}
    {\thecontentslabel\hspace{0.5em}}
    {}
    {\titlerule*[0.7pc]{.}\contentspage}
[]

% subsubsection
\titlecontents{subsubsection}
  [0.85cm]
  {}
  {\thecontentslabel\hspace{0.5em}}
  {}
  {\titlerule*[0.7pc]{.}\contentspage}
[]


% split logical structure of thesis into several chunks
\makeatletter
\renewcommand{\frontmatter}{%
  \pagestyle{plain}
  \pagenumbering{roman}
  \let\tempchapter=\chapter
  \let\chapter=\prepostchapter
}

\renewcommand{\mainmatter}{%
  \pagenumbering{arabic}
  \let\chapter=\tempchapter
  \addtocontents{toc}{\protect\contentsline {part}{Main Content}{}{}}
  \cleardoublepage
  \pagestyle{fancy}
  \fancyhead{}
  \fancyhead[LE]{\thechapter.\ \leftmark}
  \fancyhead[RO]{\thechapter.\ \leftmark}
}

\renewcommand{\backmatter}{%
  \addtocontents{toc}{\protect\contentsline {part}{Supplementary Information}{}{}}
  \let\chapter=\tempchapter
  \pagestyle{plain}
  \stopcontents
}

%\usepackage{hyperref}
\hypersetup{
  pdfsubject = {Ph.D. thesis},
  pdftitle = {\@title},
  pdfauthor = {\@author},
  pdfcreator = {LaTeX, pdfTeX},
  pdfkeywords = {astroparticle physics, ultra-high energy cosmic rays, Pierre Auger Observatory},
  colorlinks = true,
  plainpages = false,
  linkcolor = RawSienna,
  citecolor = magenta,
  urlcolor = blue,
  bookmarksopen = false,
  linktocpage = true
}

\usepackage[capitalize]{cleveref}
