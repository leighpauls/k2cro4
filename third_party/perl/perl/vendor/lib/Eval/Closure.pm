package Eval::Closure;
{
  $Eval::Closure::VERSION = '0.08';
}
use strict;
use warnings;
use Sub::Exporter -setup => {
    exports => [qw(eval_closure)],
    groups  => { default => [qw(eval_closure)] },
};
# ABSTRACT: safely and cleanly create closures via string eval

use Carp;
use overload ();
use Scalar::Util qw(reftype);
use Try::Tiny;



sub eval_closure {
    my (%args) = @_;

    $args{source} = _canonicalize_source($args{source});
    _validate_env($args{environment} ||= {});

    $args{source} = _line_directive(@args{qw(line description)})
                  . $args{source}
        if defined $args{description} && !($^P & 0x10);

    my ($code, $e) = _clean_eval_closure(@args{qw(source environment)});

    if (!$code) {
        if ($args{terse_error}) {
            die "$e\n";
        }
        else {
            croak("Failed to compile source: $e\n\nsource:\n$args{source}")
        }
    }

    return $code;
}

sub _canonicalize_source {
    my ($source) = @_;

    if (defined($source)) {
        if (ref($source)) {
            if (reftype($source) eq 'ARRAY'
             || overload::Method($source, '@{}')) {
                return join "\n", @$source;
            }
            elsif (overload::Method($source, '""')) {
                return "$source";
            }
            else {
                croak("The 'source' parameter to eval_closure must be a "
                    . "string or array reference");
            }
        }
        else {
            return $source;
        }
    }
    else {
        croak("The 'source' parameter to eval_closure is required");
    }
}

sub _validate_env {
    my ($env) = @_;

    croak("The 'environment' parameter must be a hashref")
        unless reftype($env) eq 'HASH';

    for my $var (keys %$env) {
        croak("Environment key '$var' should start with \@, \%, or \$")
            unless $var =~ /^([\@\%\$])/;
        croak("Environment values must be references, not $env->{$var}")
            unless ref($env->{$var});
    }
}

sub _line_directive {
    my ($line, $description) = @_;

    $line = 1 unless defined($line);

    return qq{#line $line "$description"\n};
}

sub _clean_eval_closure {
    my ($source, $captures) = @_;

    my @capture_keys = sort keys %$captures;

    if ($ENV{EVAL_CLOSURE_PRINT_SOURCE}) {
        _dump_source(_make_compiler_source($source, @capture_keys));
    }

    my ($compiler, $e) = _make_compiler($source, @capture_keys);
    my $code;
    if (defined $compiler) {
        $code = $compiler->(@$captures{@capture_keys});
    }

    if (defined($code) && (!ref($code) || ref($code) ne 'CODE')) {
        $e = "The 'source' parameter must return a subroutine reference, "
           . "not $code";
        undef $code;
    }

    return ($code, $e);
}

sub _make_compiler {
    my $source = _make_compiler_source(@_);

    return @{ _clean_eval($source) };
}

sub _clean_eval {
    local $@;
    local $SIG{__DIE__};
    my $compiler = eval $_[0];
    my $e = $@;
    [ $compiler, $e ];
}

$Eval::Closure::SANDBOX_ID = 0;

sub _make_compiler_source {
    my ($source, @capture_keys) = @_;
    $Eval::Closure::SANDBOX_ID++;
    my $i = 0;
    return join "\n", (
        "package Eval::Closure::Sandbox_$Eval::Closure::SANDBOX_ID;",
        'sub {',
        (map {
            'my ' . $_ . ' = ' . substr($_, 0, 1) . '{$_[' . $i++ . ']};'
         } @capture_keys),
        $source,
        '}',
    );
}

sub _dump_source {
    my ($source) = @_;

    my $output;
    if (try { require Perl::Tidy }) {
        Perl::Tidy::perltidy(
            source      => \$source,
            destination => \$output,
            argv        => [],
        );
    }
    else {
        $output = $source;
    }

    warn "$output\n";
}


1;

__END__
=pod

=head1 NAME

Eval::Closure - safely and cleanly create closures via string eval

=head1 VERSION

version 0.08

=head1 SYNOPSIS

  use Eval::Closure;

  my $code = eval_closure(
      source      => 'sub { $foo++ }',
      environment => {
          '$foo' => \1,
      },
  );

  warn $code->(); # 1
  warn $code->(); # 2

  my $code2 = eval_closure(
      source => 'sub { $code->() }',
  ); # dies, $code isn't in scope

=head1 DESCRIPTION

String eval is often used for dynamic code generation. For instance, C<Moose>
uses it heavily, to generate inlined versions of accessors and constructors,
which speeds code up at runtime by a significant amount. String eval is not
without its issues however - it's difficult to control the scope it's used in
(which determines which variables are in scope inside the eval), and it's easy
to miss compilation errors, since eval catches them and sticks them in $@
instead.

This module attempts to solve these problems. It provides an C<eval_closure>
function, which evals a string in a clean environment, other than a fixed list
of specified variables. Compilation errors are rethrown automatically.

=head1 FUNCTIONS

=head2 eval_closure(%args)

This function provides the main functionality of this module. It is exported by
default. It takes a hash of parameters, with these keys being valid:

=over 4

=item source

The string to be evaled. It should end by returning a code reference. It can
access any variable declared in the C<environment> parameter (and only those
variables). It can be either a string, or an arrayref of lines (which will be
joined with newlines to produce the string).

=item environment

The environment to provide to the eval. This should be a hashref, mapping
variable names (including sigils) to references of the appropriate type. For
instance, a valid value for environment would be C<< { '@foo' => [] } >> (which
would allow the generated function to use an array named C<@foo>). Generally,
this is used to allow the generated function to access externally defined
variables (so you would pass in a reference to a variable that already exists).

=item description

This lets you provide a bit more information in backtraces. Normally, when a
function that was generated through string eval is called, that stack frame
will show up as "(eval n)", where 'n' is a sequential identifier for every
string eval that has happened so far in the program. Passing a C<description>
parameter lets you override that to something more useful (for instance,
L<Moose> overrides the description for accessors to something like "accessor
foo at MyClass.pm, line 123").

=item line

This lets you override the particular line number that appears in backtraces,
much like the C<description> option. The default is 1.

=item terse_error

Normally, this function appends the source code that failed to compile, and
prepends some explanatory text. Setting this option to true suppresses that
behavior so you get only the compilation error that Perl actually reported.

=back

=head1 BUGS

No known bugs.

Please report any bugs through RT: email
C<bug-eval-closure at rt.cpan.org>, or browse to
L<http://rt.cpan.org/NoAuth/ReportBug.html?Queue=Eval-Closure>.

=head1 SUPPORT

You can find this documentation for this module with the perldoc command.

    perldoc Eval::Closure

You can also look for information at:

=over 4

=item * AnnoCPAN: Annotated CPAN documentation

L<http://annocpan.org/dist/Eval-Closure>

=item * CPAN Ratings

L<http://cpanratings.perl.org/d/Eval-Closure>

=item * RT: CPAN's request tracker

L<http://rt.cpan.org/NoAuth/Bugs.html?Dist=Eval-Closure>

=item * Search CPAN

L<http://search.cpan.org/dist/Eval-Closure>

=back

=head1 AUTHOR

Jesse Luehrs <doy at tozt dot net>

Based on code from L<Class::MOP::Method::Accessor>, by Stevan Little and the
Moose Cabal.

=head1 SEE ALSO

=over 4

=item * L<Class::MOP::Method::Accessor>

This module is a factoring out of code that used to live here

=back

=head1 COPYRIGHT AND LICENSE

This software is copyright (c) 2012 by Jesse Luehrs.

This is free software; you can redistribute it and/or modify it under
the same terms as the Perl 5 programming language system itself.

=cut

