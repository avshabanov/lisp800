(defmacro is (expected actual)
  (let ((a (gensym "a")) (b (gensym "b")))
    `(let ((,a ,expected) (,b ,actual))
       (if (not (eq ,a ,b))
         (progn
           (format t "FAILED: when matching ~a and ~a~%" ',expected ',actual)
           (quit 1))
         (format t "OK: ~a is ~a~%" ',expected ',actual)))))

(write-line "Running smoke test!")

(is 1 1)
(is t (equal (list 1 'a 'b) (cons 1 '(a b))))

(defun accum (r) (if (= 0 r) (list 0) (cons r (accum (- r 1)))))
(is t (equal (list 4 3 2 1 0) (accum 4)))

(defmacro defwrap (name) `(defun ,name () 1))
(defwrap foo)
(is 1 (foo))
(is t (equal (macroexpand-1 '(defwrap foo)) '(defun foo nil 1)))

(write-line "PASSED")
(quit 0)
