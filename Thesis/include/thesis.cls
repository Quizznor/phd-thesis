\ProvidesClass{include/thesis}
\LoadClass[a4paper, 11pt, twoside]{book}

\usepackage[utf8]{inputenc}
% \usepackage[T1]{fontenc}
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
\usepackage[printonlyused, footnote, withpage]{acronym}
\usepackage[dvipsnames, table]{xcolor}
\usepackage{changes}
\usepackage{blindtext}

\usepackage[b]{esvect}    % For \vv
\usepackage{tikz}         % For arrow and dots in \xvec
\usetikzlibrary{calc}

\usepackage{xparse}

% this block for listings is just for the example.tex
\usepackage{listings}
\lstset{
    frame=single,
    breaklines=true,
    postbreak=\raisebox{0ex}[0ex][0ex]{\ensuremath{\color{red}\hookrightarrow\space}}
}
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

\usepackage{color}
\definecolor{darkred}{rgb}{0.5,0,0}
\definecolor{firebrick}{rgb}{0.75,0.125,0.125}
\definecolor{darkgreen}{rgb}{0,0.5,0}
\definecolor{lightgray}{gray}{0.9}
\definecolor{gray}{rgb}{0.4,0.4,0.4}
\definecolor{darkblue}{rgb}{0.0,0.0,0.6}
\definecolor{cyan}{rgb}{0.0,0.6,0.6}

\lstset{
  basicstyle=\ttfamily,
  columns=fullflexible,
  showstringspaces=false,
  commentstyle=\color{gray}\upshape
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

%\usepackage[mathlines,switch]{lineno}

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

\usepackage[activate={true,nocompatibility},final,tracking=true,kerning=true,spacing=true,factor=1100,stretch=10,shrink=10]{microtype}
% activate={true,nocompatibility} - activate protrusion and expansion
% final - enable microtype; use "draft" to disable
% tracking=true, kerning=true, spacing=true - activate these techniques
% factor=1100 - add 10% to the protrusion amount (default is 1000)
% stretch=10, shrink=10 - reduce stretchability/shrinkability (default is 20/20)

\SetTracking{encoding={*}, shape=sc}{40}

\usepackage[Conny]{fncychap}
\ChTitleAsIs

% Paket caption muss _nach_ rotating und subfigure eingebunden werden!!
\usepackage[labelfont=bf,font=small,margin=10pt,format=plain,tableposition=top]{caption}

% from colin
\clubpenalty = 10000 % Disable single lines at the start of a paragraph (Schusterjungen)
\widowpenalty = 10000
\displaywidowpenalty = 10000 % Disable single lines at the end of a paragraph (Hurenkinder)


\usepackage[numbers,sort&compress]{natbib}

% Describe separation hints here:
\hyphenation{
  % Pro-to-koll-in-stan-zen
  % Ma-na-ge-ment  Netz-werk-ele-men-ten
  % Netz-werk Netz-werk-re-ser-vie-rung
  % Netz-werk-adap-ter Fein-ju-stier-ung
  % Da-ten-strom-spe-zi-fi-ka-tion Pa-ket-rumpf
  % Kon-troll-in-stanz
  % Me
  }

\extrafloats{100}
\usepackage{mathpazo} % use mathpazo for math fonts
  
% math mode text style automatically adjusts to surrounding style
% good snippet to make symbols in titles appear in text and bold face without defining them explicitly

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

\usepackage{titlesec}
\usepackage{titletoc}

\titlecontents{part}            % what is changed
[0.1cm]                       % left margin
{\vspace{1cm}\hrule height 1.5pt}          % code executed before toc-item
{}                            % content if label is present
{}                            % content if there is no label present
{}                            % filler page format (like dots, etc.)
[\hrule height 1.5pt]                      % code executed after toc-item

\titlecontents{chapter}
[0.2cm]
{\bfseries}
{\vspace{0.5cm}\hrule\vspace{-0.5cm}}
{}
{}

\titleformat{\chapter}[hang]{\huge\bfseries}{\thechapter\hspace{20pt}\textcolor{gray75}{|}\hspace{20pt}}{0pt}{\huge\bfseries}

\definecolor{gray75}{gray}{0.75}
\newcommand{\forceTOC}[2]{\addtocontents{toc}{\protect\contentsline {section}{\includegraphics[height=0.3cm]{../include/#1.png}\hspace{0.2cm}#2}{\color{RawSienna}{\pageref{sec:#2}}}{}}}

\makeatletter
\renewcommand{\frontmatter}{%
\titleformat*{\section}{\centering\normalfont\bfseries\Large}
\pagenumbering{roman}
}
\renewcommand{\mainmatter}{%
\addtocontents{toc}{\protect\contentsline {part}{Main Content}{}{}}
  \titleformat*{\section}{\normalfont\bfseries\Large}
  \pagenumbering{arabic}
  \setcounter{chapter}{0}
  \cleardoublepage
  }
  \renewcommand{\backmatter}{%
  \addtocontents{toc}{\protect\contentsline {part}{Supplementary Information}{}{}}
  \renewcommand{\thechapter}{\Alph{chapter}}
  \setcounter{chapter}{0}
  \cleardoublepage
  }
  
  
  
  \usepackage{hyperref}
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