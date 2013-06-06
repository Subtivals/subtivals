---
layout: main
title: Subtivals
---
![screenshot](http://mathieu-leplatre.info/media/subtivals/subtivals.png)

**Subtivals**, a program to project Advanced SubStation Alpha (ASS, *.ass), Subrip (*.srt) and simple text (*.txt) subtitles.

Purpose
=======

Hard copies for film festivals do not always carry subtitles, especially for the hard-of-hearing.

Subtivals goals are the projection and the control of subtitles superimposed on a cinema screen;
Projecting subtitles on top of another screen is called *soft-titling*, *surtitling*, *supertitling* or even sometimes *electronic subtitles*, *virtual subtitles*.

![in-situ](http://mathieu-leplatre.info/media/subtivals/subtivals-insitu.png)

Subtivals is fully automatic, but its semi-manual mode also allows to project subtitles at theaters (or opera, ballets, concerts and other cultural events...). 

## References

Subtivals was used during the following film festivals :

* [Lemesos International Documentary Festival](http://filmfestival.com.cy) in Cyprus ( *2013* )
* [Cyprus Film Days](http://cyprusfilmdays.org) ( *2013* )
* [Cinélatino de Toulouse](http://www.cinelatino.com.fr) ( *2013, 2012, 2011* )
* [Festival Cine sordo](http://www.festivalcinesordo.com) in Quito, Ecuador ( *2012* )
* [Festival de cinéma de Douarnenez](http://www.festival-douarnenez.com) ( *2012, 2011* )
* [Festival International du film d'Amiens](http://www.filmfestamiens.org) ( *2012, 2011* )
* [Résistances à Foix](http://festival-resistances.fr) ( *2012* )
* [Festival du film de Beauvais](http://www.beauvaisfilmfest.com) ( *2012* )
* [Short films festivals Vila do Conde](http://rteixeira.eu) ( *2012* )
* [Festival Zoom Arrière, 6e édition](http://www.lacinemathequedetoulouse.com/archives/2012/thematiques) ( *2012* )
* [Festival de Cinéma d'Amérique Latine de Biarritz](http://www.festivaldebiarritz.com) ( *2011* )
* [Festival Intergalactique de Brest](http://festival-galactique.infini.fr) ( *2011* )

And is being used by professionals worldwide :

* [ST2L](http://st2l.fr) ( *France* )
* [Fila13](http://fila13.org/) ( *Barcelona, Spain* )
* [Racti](http://racti.com/) ( *Lebanon* )
* [Lycée Français Sainte Pulchérie](http://www.sp.k12.tr/) ( *Istambul, Turkey* )
* [Filmoteca UNAM](http://www.filmoteca.unam.mx) ( *Mexico* )

*Contact-us to add yours!*
 
Main features
=============

* Control play/pause/delay/speed
* Switch between several modes : timecode based, semi-automatic or fully manual
* SSA styles (italic, positions, colors)
* Override styles (color, font size, margins, alignments)
* Text outline (color and width)
* Easily adjust position and size of screen window with the mouse
* Configure presets for screen positions and overriden styles
* Support of screen rotation
* Calibration mode (for both SRT and ASS)
* Support for subtitles without timecodes (fixes duration automatically)
* Support for dual head (Xinerama) setup
* Hide desktop (fullscreen) on screen monitor
* Feedback on number of characters per second
* Reload subtitles file if modified while playing
* Shortcut editor
* Runs on GNU/Linux, Windows, Mac OSX
* Available in English, French, Spanish and Catalan

Subtivals is written in C++/Qt and has no other external dependencies.

![screenshot](http://mathieu-leplatre.info/media/subtivals/subtivals-preview.png)

## Video mixer (Chroma-key) support

If you have a video mixer, then a separate projector is not necessary for inlaying subtitles !

![chromakey](http://mathieu-leplatre.info/media/subtivals/subtivals-chromakey.png)


Installation
============

Subtivals installer is primarily available on **Ubuntu**, and can be installed by adding the
following PPA to your Software Sources:

    sudo add-apt-repository ppa:mathieu.leplatre/subtivals
    sudo apt-get update && sudo apt-get install subtivals

For **Windows** and **Mac OS** installers, you can [try out the demo version](http://mathieu-leplatre.info/media/subtivals/) or contact us by email !


File Formats
============

Subtivals supports all characters and alphabets (Greek, Arabic, Chinese, ...) if the subtitles files are saved with UTF-8 encoding. 

## SSA format (*.ssa)

Supported attributes:

* Styles
* Positions
* Margins
* Colors
* Bold / Italic

## Subrip format (*.srt)

Full support.

## Text format (*.txt)

Without timecode ( *duration is computed automatically from text length* ):

```
À 24-25 ans, j'avais déjà un film
qui tournait en festival.

J'étais très jeune,

diplômé de la fac
depuis tout juste 3 ans.
```

With timecode:

```
00:00:19:13 00:00:23:08 03:20
À 24-25 ans, j'avais déjà un film
qui tournait en festival.

00:00:23:22 00:00:25:09 01:12
J'étais très jeune,

00:00:26:12 00:00:29:10 02:23
diplômé de la fac
depuis tout juste 3 ans.
```

See [example files](https://github.com/traxtech/subtivals/tree/master/examples)


Authors
=======

* Arnaud Rolly ([Gedial](http://www.gedial.com))
* Mathieu Leplatre ([@leplatrem](http://mathieu-leplatre.info))
* Lilian Lefranc ([st2l](http://st2l.fr))
* Emmanuel Digiaro ([@mallox](http://twitter.com/mallox))


See Also
========

* [Aegisub](http://www.aegisub.org), a powerful ASS subtitles editor;
* [Qstit](http://www.nova-cinema.org/spip.php?rubrique1664&lang=en), an another subtitle projection program, by Nova Cinema;
