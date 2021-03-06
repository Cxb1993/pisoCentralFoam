
volVectorField HbyA ("HbyA", U);
volScalarField rAU ("rAU", 1.0 / UEqn.A());
HbyA = UEqn.H() * rAU;

psi_pos = fvc::interpolate(psi, pos, "reconstruct(psi)");
psi_neg = fvc::interpolate(psi, neg, "reconstruct(psi)");

psiU_pos= fvc::interpolate(psi*HbyA, pos, "reconstruct(U)");
psiU_neg= fvc::interpolate(psi*HbyA, neg, "reconstruct(U)");

phiv_pos= (psiU_pos / psi_pos) & mesh.Sf();
phiv_neg= (psiU_neg / psi_neg) & mesh.Sf();

c = sqrt(thermo.Cp()/thermo.Cv() / psi);

cSf_pos = fvc::interpolate(c, pos, "reconstruct(psi)")*mesh.magSf();
cSf_neg = fvc::interpolate(c, neg, "reconstruct(psi)")*mesh.magSf();

ap = max(max(phiv_pos - fvc::meshPhi(rho,U) + cSf_pos, phiv_neg - fvc::meshPhi(rho,U) + cSf_neg), v_zero);
am = min(min(phiv_pos - fvc::meshPhi(rho,U) - cSf_pos, phiv_neg - fvc::meshPhi(rho,U) - cSf_neg), v_zero);

aphiv_pos = a_pos * (phiv_pos - fvc::meshPhi(rho,U)) - aSf;
aphiv_neg = a_neg * (phiv_neg - fvc::meshPhi(rho,U)) + aSf;

a_pos = ap/(ap - am);
aSf = am*a_pos;
a_neg = 1.0 - a_pos;

phid_pos = aphiv_pos * psi_pos;
phid_neg = aphiv_neg * psi_neg;

surfaceScalarField Dp_pos
(
    "Dp_pos",
    fvc::interpolate(rho*rAU, pos, "reconstruct(Dp)")
);

surfaceScalarField Dp_neg
(
    "Dp_neg",
    fvc::interpolate(rho*rAU, neg, "reconstruct(Dp)")
);

while (pimple.correctNonOrthogonal())
{
    fvScalarMatrix pEqn_pos
    (
	fvm::div(phid_pos,p) - fvm::laplacian(Dp_pos*a_pos,p)
    );
    
    fvScalarMatrix pEqn_neg
    (
	fvm::div(phid_neg,p) - fvm::laplacian(Dp_neg*a_neg,p)
    );
    
    fvScalarMatrix pEqn
    (
	fvm::ddt(psi,p)
	+
	pEqn_pos
	+
	pEqn_neg
    );
    
    pEqn.solve(mesh.solver(p.select(pimple.finalInnerIter())));
    
    if (pimple.finalNonOrthogonalIter())
    {
	phiPos = pEqn_pos.flux();
	phiNeg = pEqn_neg.flux();
	phi = phiPos + phiNeg;
    }
}

p_pos = fvc::interpolate(p, pos, "reconstruct(p)");
p_neg = fvc::interpolate(p, neg, "reconstruct(p)");

gradp = fvc::div((a_pos*p_pos + a_neg*p_neg)*mesh.Sf());
#include "limitPressureGradient.H"
U = HbyA - rAU * gradp;
U.correctBoundaryConditions();

Info << "max(U): " << max(U).value() << endl;
solve
(
    fvm::ddt(rho) + fvc::div(phi)
);

#include "compressibleContinuityErrs.H"

rho = thermo.rho();
