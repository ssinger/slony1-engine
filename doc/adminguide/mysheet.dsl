<!DOCTYPE style-sheet PUBLIC "-//James Clark//DTD DSSSL Style Sheet//EN" [
<!--  /usr/lib/sgml/stylesheet/dsssl/docbook/nwalsh/print/docbook.dsl  -->
<!-- <!ENTITY mydbstyle SYSTEM 
     "/usr/lib/sgml/stylesheet/dsssl/docbook/nwalsh/print/docbook.dsl"
     CDATA DSSSL> -->
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA DSSSL>
<!ENTITY % output.html  "IGNORE">
<!ENTITY % output.print "IGNORE">
<!ENTITY % lang.ja      "IGNORE">
<!ENTITY % lang.ja.dsssl        "IGNORE">
<![ %output.html; [
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook HTML Stylesheet//EN" CDATA DSSSL>
]]>
<![ %output.print; [
<!ENTITY docbook.dsl PUBLIC "-//Norman Walsh//DOCUMENT DocBook Print Stylesheet//EN" CDATA DSSSL>
]]>
]>

<style-sheet>
  <style-specification id="html" use="docbook">
    <style-specification-body>
      <!-- Locatization -->
      <![ %lang.ja; [
        <![ %lang.ja.dsssl; [
          (define %gentext-language% "ja")
        ]]>
        (define %html-header-tags% 
    '(("META" ("HTTP-EQUIV" "Content-Type") 
       ("CONTENT" "text/html; charset=EUC-JP"))
     ;;; ("BASE" ("HREF" "http://linuxdatabases.info/info/index.html"))))
     ;;; ("BASE" ("HREF" "http://www3.sympatico.ca/cbbrowne/index.html"))))
       )
      ]]>

      <!-- HTML only .................................................... -->
      
      <![ %output.html; [
        <!-- Configure the stylesheet using documented variables -->

        (define %gentext-nav-use-tables% #t)

        (define %html-ext% ".html")

        (define %shade-verbatim% #t)



;(define (graphic-attrs imagefile instance-alt)
;   (let* ((grove
;	   (sgml-parse image-library-filename))
;	  (my-node-list-first (lambda (x) (if x (node-list-first x) #f)))
;	  (imagelib
;	   (node-property 'document-element 
;			  (node-property 'grove-root grove)))
;	  (images
;	   (select-elements (children imagelib) "image"))
;	  (image
;	   (let loop ((imglist images))
;	      (if (and imglist (node-list-empty? imglist))
;		  #f
;		  (if (equal? (attribute-string 
;			       "filename"
;			       (node-list-first imglist))
;			      imagefile)
;		      (node-list-first imglist)
;		      (loop (node-list-rest imglist))))))
;	  (prop
;	   (if image
;	       (select-elements (children image) "properties")
;	       #f))
;	  (metas
;	   (if prop 
;	       (select-elements (children prop) "meta")
;	       #f))
;	  (attrs
;	   (let loop ((meta metas) (attrlist '()))
;	      (if (and meta (node-list-empty? meta))
;		  attrlist
;		  (if (equal? (attribute-string 
;			       "imgattr"
;			       (node-list-first meta))
;			      "yes")
;		      (loop (node-list-rest meta)
;			    (append attrlist
;				    (list 
;				     (list 
;				      (attribute-string 
;				       "name"
;				       (node-list-first meta))
;				      (attribute-string
;				       "content"
;				       (node-list-first meta))))))
;		      (loop (node-list-rest meta) attrlist)))))
;	  (width    (attribute-string "width" prop))
;	  (height   (attribute-string "height" prop))
;	  (alttext  (select-elements (children image) "alttext"))
;	  (alt      (if instance-alt 
;			instance-alt 
;			(if (and alttext (node-list-empty? alttext))
;			    #f
;			    (data alttext)))))
;      (if (or width height alt (not (null? attrs)))
;	  (append
;	   attrs
;	   (if width   (list (list "WIDTH" width)) '())
;	   (if height  (list (list "HEIGHT" height)) '())
;	   (if alttext (list (list "ALT" alt)) '()))
;	  '())))

        (define %use-id-as-filename% #t)
          ;; Use ID attributes as name for component HTML files?
 
        ;; Name for the root HTML document
        (define %root-filename% #f)

        ;; Write a manifest?
        (define html-manifest #f)

	(define %author-othername-in-middle% #t)
	(define %admon-graphics% #t)
	(define %admon-graphics-path% "./images/")
        (define %body-attr%
          (list 
            (list "BGCOLOR" "#FFFFFF")
            (list "TEXT" "#000000")
            (list "LINK" "#0000FF")
	    (list "VLINK" "#840084")
	    (list "ALINK" "#0000FF")))

	(define %html-header-tags% 
          '(("META" ("HTTP-EQUIV" "Content-Type"))
            ;;; ("BASE" ("HREF" "http://linuxfinances.info/info/index.html"))))
            ))

        (define %html40% #t)
	(define %stylesheet% "stdstyle.css")
	(define %stylesheet-type% "text/css")
	(define %css-decoration% #t)
	(define %css-liststyle-alist%
		'(("bullet" "disc") ("box" "square")))
	(define %spacingparas% #t)
	(define %link-mailto-url% "mailto:cbbrowne@gmail.com")
	(define %generate-article-toc% #t)
	(define %generate-reference-toc% #t)
	(define %generate-reference-toc-on-titlepage% #t)
	(define %annotate-toc% #t)
	(define %generate-set-titlepage% #t)
	(define %generate-book-titlepage% #t)
	(define %generate-part-titlepage% #t)
	(define %generate-reference-titlepage% #t)
	(define %generate-article-titlepage% #t)
        (define %emphasis-propagates-style% #t)

	<!-- Add in image-library -->
	(define image-library #t)
	(define image-library-filename "imagelib/imagelib.xml")

        <!-- Understand <segmentedlist> and related elements.  Simpleminded,
             and only works for the HTML output. -->

        (element segmentedlist
          (make element gi: "TABLE"
            (process-children)))

        (element seglistitem
          (make element gi: "TR"
            (process-children)))

        (element seg
          (make element gi: "TD"
                attributes: '(("VALIGN" "TOP"))
            (process-children)))

      ]]>

      <!-- Print only ................................................... --> 
      <![ %output.print; [

      ]]>

      <!-- Both sets of stylesheets ..................................... -->

      (define %section-autolabel%
        #t)

      (define %may-format-variablelist-as-table%
        #f)
      
      (define %indent-programlisting-lines%
        "    ")
 
      (define %indent-screen-lines%
        "    ")
      
      <!-- Slightly deeper customisations -->

      <!-- I want things marked up with 'sgmltag' eg., 

              <para>You can use <sgmltag>para</sgmltag> to indicate
                paragraphs.</para>

           to automatically have the opening and closing braces inserted,
           and it should be in a mono-spaced font. -->

      (element sgmltag ($mono-seq$
          (make sequence
            (literal "<")
            (process-children)
            (literal ">"))))

      <!-- John Fieber's 'instant' translation specification had 
           '<command>' rendered in a mono-space font, and '<application>'
           rendered in bold. 

           Norm's stylesheet doesn't do this (although '<command>' is 
           rendered in bold).

           Configure the stylesheet to behave more like John's. -->

      (element command ($mono-seq$))

      (element application ($bold-seq$))

      <!-- Warnings and cautions are put in boxed tables to make them stand
           out. The same effect can be better achieved using CSS or similar,
           so have them treated the same as <important>, <note>, and <tip>
      -->
      (element warning ($admonition$))
      (element (warning title) (empty-sosofo))
      (element (warning para) ($admonpara$))
      (element (warning simpara) ($admonpara$))
      (element caution ($admonition$))
      (element (caution title) (empty-sosofo))
      (element (caution para) ($admonpara$))
      (element (caution simpara) ($admonpara$))

      (define en-warning-label-title-sep ": ")
      (define en-caution-label-title-sep ": ")

      <!-- Tell the stylesheet about our local customisations -->
      
      (element hostid ($mono-seq$))
      (element username ($mono-seq$))
      (element devicename ($mono-seq$))
      (element maketarget ($mono-seq$))
      (element makevar ($mono-seq$))

      <!-- QAndASet ..................................................... -->

      <!-- Default to labelling Q/A with Q: and A: -->

      (define (qanda-defaultlabel)
        (normalize "qanda"))

      <!-- For the HTML version, display the questions in a bigger, bolder
           font. -->

      <![ %output.html [
        ;; Custom function because of gripes about node-list-first

      (element question
        (let* ((chlist   (children (current-node)))
               (firstch  (node-list-first chlist))
               (restch   (node-list-rest chlist)))
               (make element gi: "DIV"
                     attributes: (list (list "CLASS" (gi)))
                     (make element gi: "P" 
                           (make element gi: "BIG"
                                 (make element gi: "A"
                                       attributes: (list
                                                   (list "NAME" (element-id)))
                                       (empty-sosofo))
                                 (make element gi: "B"
                                       (literal (question-answer-label
                                                (current-node)) " ")
                                       (process-node-list (children firstch)))))
                    (process-node-list restch))))
      ]]>

    </style-specification-body>
  </style-specification>
  <external-specification  id="docbook" document="docbook.dsl">
</style-sheet>


<!--  /usr/lib/sgml/stylesheet/dsssl/docbook/nwalsh/html/dbparam.dsl -->
