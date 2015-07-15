;
; Simple crisp deduction example
;
; 1. Launch guile under this directory (see examples/guile/README.md
; for prerequisite if you haven't already)
;
; $ guile
;
; 2. The load this file
;
; scheme@(guile-user)> (load-from-path "crisp-deduction.scm")
;
; 3. Scroll to the bottom, and run some of the commented-out examples.

(use-modules (opencog))
(use-modules (opencog rule-engine))

; Load URE configuration (add the current file dir so it can be loaded
; from anywhere)
(add-to-load-path (dirname (current-filename)))
(load-from-path "crisp-deduction-config.scm")

; Define knowledge base
(define A (ConceptNode "A" (stv 1 1)))
(define B (ConceptNode "B"))
(define C (ConceptNode "C"))
(define AB (ImplicationLink (stv 1 1) A B))
(define BC (ImplicationLink (stv 1 1) B C))

; 1. Test forward chaining (based on the deduction rule)

; (crisp-deduction-fc AB)

; Expected output should be something like
;; $1 = (ListLink
;;    (ImplicationLink (stv 1 0.99999982)
;;       (ConceptNode "A")
;;       (ConceptNode "C")
;;    )
;; )

; 2. Test backward chaining (you need to quite guile and restart to be
; sure the results you're getting aren't provided by the forward
; chainer command above)

; (crisp-deduction-bc AC)

; Expected output will be empty
;; $1 = (ListLink
;; )
;; while the TV of AC will be suitably updated.